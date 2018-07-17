#include "player.hpp"
#include <cmath>


namespace player {
namespace {


struct Channel {
    bool active = true;

    enum State { OFF, RELEASE, ATTACK, HOLD };
    enum Wave { PULSE, SAW, TRIANGLE, NOISE };

    // voice stuff
    State    state;
    float    level;
    float    phase;
    float    speed;
    uint32_t shift;

    Wave     wave = PULSE;
    float    pulsewidth = 0.5;

    float attack  = 0.01;
    float decay   = 0.9999;
    float sustain = 0.7;
    float release = 0.99;
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
        Tune::Block const& block = m_tune.table[m_block];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            int track_nr = block[c];
            if (track_nr == 0) continue;
            Track const& track = m_tune.tracks[track_nr - 1];
            Track::Row const& row = track.rows[m_row];

            if (row.note > 0) {
                // new note
                float pitch = row.note;
                chan.speed = exp2f((pitch - 57) / 12) * 440 / MIXRATE;
                chan.state = Channel::ATTACK;
            }
            else if (row.note == -1) {
                chan.state = Channel::RELEASE;
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

        float sample = 0;
        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            Channel& chan = m_channels[c];
            if (!chan.active) continue;

            switch (chan.state) {
            case Channel::OFF: continue;
            case Channel::ATTACK:
                chan.level += chan.attack;
                if (chan.level > 1) chan.state = Channel::HOLD;
                break;
            case Channel::HOLD: chan.level = chan.sustain + (chan.level - chan.sustain) * chan.decay; break;
            case Channel::RELEASE:
            default: chan.level *= chan.release; break;
            }

            chan.phase += chan.speed;
            if (chan.wave != Channel::NOISE) chan.phase -= (int) chan.phase;

            float amp = 0;

            switch (chan.wave) {
            case Channel::PULSE:
                amp = chan.phase < chan.pulsewidth ? -1 : 1;
                break;
            case Channel::TRIANGLE:
                amp = chan.phase < chan.pulsewidth ?
                    2 / chan.pulsewidth * chan.phase - 1 :
                    2 / (chan.pulsewidth - 1) * (chan.phase - chan.pulsewidth) + 1;
                break;
            case Channel::NOISE: {
                uint32_t s = chan.shift;
                uint32_t b;
                while (chan.phase > 0.1) {
                    chan.phase -= 0.1;
                    b = ((s >> 22) ^ (s >> 17)) & 1;
                    s = ((s << 1) & 0x7fffff) + b;
                }
                chan.shift = s;
                amp = (
                        ((s & 0x400000) >> 11) |
                        ((s & 0x100000) >> 10) |
                        ((s & 0x010000) >> 7) |
                        ((s & 0x002000) >> 5) |
                        ((s & 0x000800) >> 4) |
                        ((s & 0x000080) >> 1) |
                        ((s & 0x000010) << 1) |
                        ((s & 0x000004) << 2)) * (2.0 / (1<<12)) - 1;
                break;
            }
            default: break;
            }

            amp *= chan.level;
            sample += amp;
        }

        buffer[i] = std::max(-32768, std::min<int>(sample * 6000, 32767));
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
    for (Channel& chan : m_channels) chan.state = Channel::OFF;
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
