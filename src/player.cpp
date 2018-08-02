#include "player.hpp"
#include <cmath>


namespace player {
namespace {

constexpr Filter     null_filter     = {};
constexpr Instrument null_instrument = {};
constexpr Effect     null_effect     = {};

constexpr std::array<int, 16> attack_speeds = {
    168867, 47495, 24124, 15998, 10200, 6908, 5692, 4855,
    3877, 1555, 777, 486, 389, 129, 77, 48,
};

constexpr std::array<int, 16> release_speeds = {
    42660, 15468, 7857, 5210, 3322, 2250, 1853, 1581,
    1262, 506, 253, 158, 126, 42, 25, 15,
};


enum State { RELEASE, ATTACK, DECAY, SUSTAIN };


struct Channel {
    bool              active = true;

    int               note;
    bool              gate;
    Instrument const* inst = &null_instrument;
    int               inst_row;
    Effect const*     effect = &null_effect;
    int               effect_row;

    // TODO
    //int               inst_pw  = 0;


    State    state;
    int      adsr[4];
    int      flags;
    uint32_t pulsewidth;
    uint32_t freq;


    // internal things
    int      level;
    uint32_t phase;
    uint32_t noise_phase;
    uint32_t shift = 0x7ffff8;
    int      noise;
};


struct {
    Filter const* filter = &null_filter;
    int           row;

    uint8_t type;
    int     resonance;
    int     freq;

    int high;
    int band;
    int low;

} m_filter;

Tune m_tune;
bool m_playing;
int  m_sample;
int  m_frame;
int  m_row;
int  m_block;
bool m_block_loop;
std::array<Channel, CHANNEL_COUNT> m_channels;


void tick() {
    if (!m_playing) return;

    // row_update
    if (m_frame == 0) {
        int block_nr = m_block;
        if (m_block >= (int) m_tune.table.size()) block_nr = 0;
        m_block = block_nr;

        Tune::Block const& block = m_tune.table[block_nr];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            int track_nr = block[c];
            if (track_nr == 0) continue;
            Track const& track = m_tune.tracks[track_nr - 1];
            Track::Row const& row = track.rows[m_row];

            // instrument
            if (row.instrument > 0) {
                chan.inst = &m_tune.instruments[row.instrument - 1];
                Instrument const& inst = *chan.inst;
                chan.adsr[0] = attack_speeds[inst.adsr[0]];
                chan.adsr[1] = release_speeds[inst.adsr[1]];
                chan.adsr[2] = inst.adsr[2] * 0x111111;
                chan.adsr[3] = release_speeds[inst.adsr[3]];
                chan.inst_row = 0;
                chan.gate = true;

                // cause envelop reset
                // XXX: do we want that?
                chan.state = RELEASE;

                // filter
                if (inst.filter.length > 0) {
                    m_filter.filter = &inst.filter;
                    m_filter.row = 0;
                }
            }

            // effect
            if (row.effect > 0) {
                chan.effect = &m_tune.effects[row.effect - 1];
                chan.effect_row = 0;
            }

            // note
            if (row.note == 255) {
                chan.gate = false;
            }
            else if (row.note > 0) {
                chan.note = row.note;
                chan.freq = exp2f((chan.note - 58) / 12.0f) * (1 << 28) * 440 / MIXRATE;
            }
        }
    }

    // frame update
    for (int c = 0; c < CHANNEL_COUNT; ++c) {
        Channel& chan = m_channels[c];

        // instrument
        Instrument const& inst = *chan.inst;
        if (inst.length > 0) {
            if (chan.inst_row >= inst.length) {
                chan.inst_row = std::min<int>(inst.loop, inst.length - 1);
            }
            Instrument::Row const& row = inst.rows[chan.inst_row++];
            chan.flags = row.flags;
            switch (row.operation) {
            case OP_SET:
                chan.pulsewidth = row.value * 0x800000;
                break;
            case OP_INC:
                chan.pulsewidth = (chan.pulsewidth + row.value * 0x40000) & 0xfffffff;
                break;
            default:
                break;
            }
        }

        // effect
        Effect const& effect = *chan.effect;
        if (effect.length > 0) {
            if (chan.effect_row >= effect.length) {
                chan.effect_row = std::min<int>(effect.loop, effect.length - 1);
            }
            int offset = effect.rows[chan.effect_row++] - 0x80;
            chan.freq = exp2f((chan.note - 58 + offset * 0.25f) / 12.0f) * (1 << 28) * 440 / MIXRATE;
        }
    }

    // filter
    Filter const& filter = *m_filter.filter;
    if (filter.length > 0) {
        if (m_filter.row >= filter.length) {
            m_filter.row = std::min<int>(filter.loop, filter.length - 1);
        }
        auto& row = filter.rows[m_filter.row++];
        m_filter.type = row.type;
        switch (row.operation) {
        case OP_SET:
            break;
        case OP_DEC:
            break;
        case OP_INC:
            break;
        default:
            break;
        }

        // TODO: set freq + reso
    }




    // advance
    int frames_per_row = m_tune.tempo;
    if (m_row % 2 == 0) frames_per_row += m_tune.swing;

    if (++m_frame >= frames_per_row) {
        m_frame = 0;
        if (++m_row >= TRACK_LENGTH) {
            m_row = 0;
            if (!m_block_loop && ++m_block >= (int) m_tune.table.size()) {
                m_block = 0;
            }
        }
    }
}


void mix(short* buffer, int length) {
    for (int i = 0; i < length; ++i) {

        int sample = 0;

        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            Channel& prev_chan = m_channels[c == 0 ? CHANNEL_COUNT - 1 : c - 1];
            if (!chan.active) continue;

            bool gate = chan.gate && (chan.flags & GATE);
            if (gate && chan.state == RELEASE) chan.state = ATTACK;
            if (!gate) chan.state = RELEASE;

            switch (chan.state) {
            case ATTACK:
                chan.level += chan.adsr[0];
                if (chan.level >= 0xffffff) {
                    chan.level = 0xffffff;
                    chan.state = DECAY;
                }
                break;
            case DECAY:
                chan.level -= chan.adsr[1];
                if (chan.level <= chan.adsr[2]) {
                    chan.level = chan.adsr[2];
                    chan.state = SUSTAIN;
                }
                break;
            case SUSTAIN:
                if (chan.level != chan.adsr[2]) chan.state = ATTACK;
                break;
            case RELEASE:
                chan.level -= chan.adsr[3];
                if (chan.level < 0) chan.level = 0;
                break;
            }

            chan.phase += chan.freq;
            chan.phase &= 0xfffffff;

            // sync
            if (chan.flags & SYNC) {
                if (prev_chan.phase < prev_chan.freq) {
                    chan.phase = prev_chan.phase * chan.freq / prev_chan.freq;
                }
            }

            // waveforms
            uint8_t tri   = ((chan.phase < 0x8000000 ? chan.phase : ~chan.phase) >> 19) & 0xff;
            uint8_t saw   = (chan.phase >> 20) & 0xff;
            uint8_t pulse = ((chan.phase > chan.pulsewidth) - 1) & 0xff;
            if (chan.noise_phase != chan.phase >> 23) {
                chan.noise_phase = chan.phase >> 23;
                uint32_t s = chan.shift;
                chan.shift = s = (s << 1) | (((s >> 22) ^ (s >> 17)) & 1);
                chan.noise = ((s & 0x400000) >> 11) |
                             ((s & 0x100000) >> 10) |
                             ((s & 0x010000) >>  7) |
                             ((s & 0x002000) >>  5) |
                             ((s & 0x000800) >>  4) |
                             ((s & 0x000080) >>  1) |
                             ((s & 0x000010) <<  1) |
                             ((s & 0x000004) <<  2);
            }
            uint8_t noise = chan.noise;

            // ringmod
            if (chan.flags & RING && prev_chan.phase < 0x8000000) tri = ~tri;

            uint8_t out = 0xff;
            if (chan.flags & TRI)   out &= tri;
            if (chan.flags & SAW)   out &= saw;
            if (chan.flags & PULSE) out &= pulse;
            if (chan.flags & NOISE) out &= noise;

            sample += ((out - 0x80) * chan.level) >> 18;
        }



        // filter


        buffer[i] = std::max(-32768, std::min<int>(sample, 32767));
    }
}


} // namespace


void fill_buffer(short* buffer, int length) {
    while (length > 0) {
        if (m_sample == 0) tick();
        int l = std::min(SAMPLES_PER_FRAME - m_sample, length);
        m_sample += l;
        if (m_sample == SAMPLES_PER_FRAME) m_sample = 0;
        length -= l;
        mix(buffer, l);
        buffer += l;
    }
}


void play() {
    m_playing = true;
}


void pause() {
    m_playing = false;
    for (Channel& chan : m_channels) {
        chan.gate   = false;
        chan.level  = 0;
    }
}


void stop() {
    pause();
    if (!m_block_loop) m_block = 0;
    m_row = 0;
    m_frame = 0;
}


int   row() { return m_row; }
int   block() { return m_block; }
void  block(int b) { m_block = b; }
bool  block_loop() { return m_block_loop; }
void  block_loop(bool b) { m_block_loop = b; }
bool  is_playing() { return m_playing; }
bool  is_channel_active(int c) { return m_channels[c].active; }
void  set_channel_active(int c, bool a) { m_channels[c].active = a; }
Tune& tune() { return m_tune; }


} // namespace;
