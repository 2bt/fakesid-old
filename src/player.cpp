#include "player.hpp"
#include <cmath>


namespace player {
namespace {


constexpr std::array<int, 16> attack_speeds = {
    168867, 47495, 24124, 15998, 10200, 6908, 5692, 4855,
    3877, 1555, 777, 486, 389, 129, 77, 48,
};

constexpr std::array<int, 16> release_speeds = {
    42660, 15468, 7857, 5210, 3322, 2250, 1853, 1581,
    1262, 506, 253, 158, 126, 42, 25, 15,
};


enum State { RELEASE, ATTACK, DECAY, SUSTAIN };
enum Wave { TRI = 1, SAW = 2, PULSE = 4, NOISE = 8 };


struct Channel {
    bool active = true;
    // voice stuff
    State    state;
    int      level;
    uint8_t  adsr[4] = {0, 8, 8, 3};
    uint32_t phase;
    uint32_t freq;
    uint32_t noise_phase;
    uint32_t shift = 0x7ffff8;
    uint8_t  noise;
    int      wave  = SAW;
    int      pulse = 0x8000000;
};


Tune m_tune;
bool m_playing;
int  m_sample;
int  m_frame;
int  m_row;
int  m_block;
std::array<Channel, CHANNEL_COUNT> m_channels;


void tick() {
    if (!m_playing) return;
    if (m_frame == 0) {
        if (m_block >= m_tune.table.size()) m_block = 0;

        Tune::Block const& block = m_tune.table[m_block];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            int track_nr = block[c];
            if (track_nr == 0) continue;
            Track const& track = m_tune.tracks[track_nr - 1];
            Track::Row const& row = track.rows[m_row];

            if (row.note > 0) {
                // new note
                chan.state = ATTACK;
                chan.freq  = exp2f((row.note - 58) / 12.0f) * (1 << 28) * 440 / MIXRATE;
            }
            else if (row.note == -1) {
                chan.state = RELEASE;
            }
        }
    }

    int frames_per_row = m_tune.tempo;
    if (m_row % 2 == 0) frames_per_row += m_tune.swing;

    if (++m_frame >= frames_per_row) {
        m_frame = 0;
        if (++m_row >= TRACK_LENGTH) {
            m_row = 0;
            if (++m_block >= m_tune.table.size()) {
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
            if (!chan.active) continue;

            int sustain = chan.adsr[2] * 0x111111;

            switch (chan.state) {
            case ATTACK:
                chan.level += attack_speeds[chan.adsr[0]];
                if (chan.level >= 0xffffff) {
                    chan.level = 0xffffff;
                    chan.state = DECAY;
                }
                break;
            case DECAY:
                chan.level -= release_speeds[chan.adsr[1]];
                if (chan.level <= sustain) {
                    chan.level = sustain;
                    chan.state = SUSTAIN;
                }
                break;
            case SUSTAIN:
                if (chan.level != sustain) chan.state = ATTACK;
                break;
            case RELEASE:
                chan.level -= release_speeds[chan.adsr[3]];
                if (chan.level < 0) chan.level = 0;
                break;
            }

            chan.phase += chan.freq;
            chan.phase &= 0xfffffff;

            uint8_t tri   = ((chan.phase < 0x8000000 ? chan.phase : ~chan.phase) >> 19) & 0xff;
            uint8_t saw   = (chan.phase >> 20) & 0xff;
            uint8_t pulse = ((chan.phase > chan.pulse) - 1) & 0xff;
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

            uint8_t out = 0xff;
            if (chan.wave & TRI)   out &= tri;
            if (chan.wave & SAW)   out &= saw;
            if (chan.wave & PULSE) out &= pulse;
            if (chan.wave & NOISE) out &= noise;

            sample += ((out - 0x80) * chan.level) >> 18;
        }

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
    for (Channel& chan : m_channels) chan.state = RELEASE;
}


void stop() {
    pause();
    m_block = 0;
    m_row = 0;
    m_frame = 0;
}


int row() { return m_row; }


int block() { return m_block; }


bool is_playing() { return m_playing; }


bool is_channel_active(int c) { return m_channels[c].active; }


void set_channel_active(int c, bool a) {
    m_channels[c].active = a;
}


Tune& tune() { return m_tune; }


} // namespace;
