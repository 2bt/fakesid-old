#include "game.hpp"
#include "gui.hpp"
#include <vector>


namespace game {
namespace {


enum {
    MIXRATE           = 44100,
    SAMPLES_PER_FRAME = MIXRATE / 50,
    CHANNEL_COUNT     = 4,
    TRACK_LENGTH      = 32,
    TRACK_COUNT       = 256,
    PAGE_LENGTH       = 16,
};


struct Track {
    struct Row {
        int instrument;
        int effect;
        int note;
    };
    std::array<Row, TRACK_LENGTH> rows;
//    int length;
};


struct Tune {
    int tempo; // 4 to F
    int swing; // 0 to 4

    std::array<Track, TRACK_COUNT> tracks;


    using Block = std::array<int, CHANNEL_COUNT>;
    std::vector<Block> table;

} m_tune;


class Player {
public:
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
        m_row = 0;
        m_frame = 0;
    }

    int  get_row() const { return m_row; }
    bool is_playing() const { return m_playing; }


private:
    void tick() {
        if (!m_playing) return;
        if (m_frame == 0) {
            Tune::Block const& block = m_tune.table[m_block];

            for (int c = 0; c < CHANNEL_COUNT; ++c) {
                Channel& chan = m_channels[c];
                Track const& track = m_tune.tracks[block[c]];
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
            }
        }
    }
    void mix(short* buffer, int length) {
        for (int i = 0; i < length; ++i) {

            float sample = 0;
            for (int c = 0; c < CHANNEL_COUNT; ++c) {
                Channel& chan = m_channels[c];

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

            buffer[i] = glm::clamp<int>(sample * 6000, -32768, 32767);
        }
    }


    struct Channel {
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


    bool m_playing;
    int  m_sample;
    int  m_frame;
    int  m_row;
    int  m_block;
    std::array<Channel, CHANNEL_COUNT> m_channels;
} m_player;


void audio_callback(void* userdata, Uint8* stream, int len) {
    m_player.fill_buffer((short*) stream, len / 2);
}


struct Edit {
    int clavier_offset = 48;
    int track_page     = 0;

    int instrument = 1;
    int effect     = 2;
} m_edit;


char* m_pref_path;


} // namespace


bool save_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "wb");
    if (!file) return false;
    SDL_RWwrite(file, &m_tune.tempo, sizeof(int), 1);
    SDL_RWwrite(file, &m_tune.swing, sizeof(int), 1);
    SDL_RWwrite(file, m_tune.tracks.data(), sizeof(Track), m_tune.tracks.size());
    int table_len = m_tune.table.size();
    SDL_RWwrite(file, &table_len, sizeof(int), 1);
    SDL_RWwrite(file, m_tune.table.data(), sizeof(Tune::Block), table_len);
    SDL_RWclose(file);
    return true;
}


bool load_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (!file) return false;
    SDL_RWread(file, &m_tune.tempo, sizeof(int), 1);
    SDL_RWread(file, &m_tune.swing, sizeof(int), 1);
    SDL_RWread(file, m_tune.tracks.data(), sizeof(Track), m_tune.tracks.size());
    int table_len;
    SDL_RWread(file, &table_len, sizeof(int), 1);
    m_tune.table.resize(table_len);
    SDL_RWread(file, m_tune.table.data(), sizeof(Tune::Block), table_len);
    SDL_RWclose(file);
    return true;
}


bool init() {
    m_pref_path = SDL_GetPrefPath("sdl", "rausch");
    if (!m_pref_path) return false;

    // default tune
    m_tune.tempo = 5;
    m_tune.table.emplace_back(Tune::Block{ 1, 0, 0, 0 });
    Track& track = m_tune.tracks[1];
    track.rows[0] = { 0, 0, 37 };
    track.rows[2] = { 0, 0, -1 };
    track.rows[4] = { 0, 0, 49 };
    track.rows[6] = { 0, 0, -1 };

    // try to load last tune
//    load_tune("tune");


	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
	SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);
    return true;
}


void free() {
//    save_tune("tune");
    SDL_free(m_pref_path);
}


void track_view() {
    gfx::font(FONT_DEFAULT);

    if (gui::button("save")) save_tune("tune");
    gui::same_line();
    if (gui::button("load")) load_tune("tune");

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    enum { COLS = 17 };
    gui::drag_int("clavier", m_edit.clavier_offset, 0, 96 - COLS, COLS);

    int player_row = m_player.get_row();
    Track& track = m_tune.tracks[1];

    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int row_nr = m_edit.track_page * PAGE_LENGTH + i;
        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4] = ".";

        // instrument
        if (row.instrument > 0) str[0] = '0' - 1  + row.instrument;
        gui::min_item_size({ 0, 62 });
        if (gui::button(str, highlight)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = m_edit.instrument;
        }

        // effect
        str[0] = '.';
        if (row.effect > 0) str[0] = '0' - 1  + row.effect;
        gui::same_line();
        gui::min_item_size({ 0, 62 });
        if (gui::button(str, highlight)) {
            if (row.effect > 0) row.effect = 0;
            else row.effect = m_edit.effect;
        }


        // note
        str[0] = str[1] = str[2] = '.';
        if (row.note > 0) {
            str[0] = "CCDDEFFGGAAB"[(row.note - 1) % 12];
            str[1] = "-#-#--#-#-#-"[(row.note - 1) % 12];
            str[2] = '0' + (row.note - 1) / 12;
        }
        if (row.note == -1) {
            str[0] = str[1] = str[2] = '=';
        }
        gui::min_item_size({ 0, 62 });
        gui::same_line();
        if (gui::button(str, highlight)) {
            if (row.note == 0) row.note = -1;
            else row.note = 0;
        }

        // clavier
        gui::same_line();
        gui::clavier(row.note, m_edit.clavier_offset, highlight);
    }

    gfx::font(FONT_DEFAULT);

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::drag_int("track", m_edit.track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);

    if (gui::button("play")) m_player.play();
    gui::same_line();
    if (gui::button("pause")) m_player.pause();
    gui::same_line();
    if (gui::button("stop")) m_player.stop();

}


void draw() {
    gfx::clear();

//    {
//        SDL_SetRenderDrawColor(gfx::renderer(), 0, 100, 200, 50);
//        auto s = gfx::screensize();
//        SDL_Rect rect = { 5, 5, s.x - 10, s.y - 10 };
//        SDL_RenderFillRect(gfx::renderer(), &rect);
//    }


    gui::begin_frame();

    static int tick = 0;
    gfx::font(FONT_MONO);
    gui::text("%d", tick++);

    track_view();


    gfx::present();
}




} // namespace
