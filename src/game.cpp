#include "game.hpp"
#include "gui.hpp"
#include "player.hpp"


namespace game {
namespace {


char* m_pref_path;


bool save_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "wb");
    if (!file) return false;
    Tune& t = player::tune();
    SDL_RWwrite(file, &t.tempo, sizeof(int), 1);
    SDL_RWwrite(file, &t.swing, sizeof(int), 1);
    SDL_RWwrite(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    int table_len = t.table.size();
    SDL_RWwrite(file, &table_len, sizeof(int), 1);
    SDL_RWwrite(file, t.table.data(), sizeof(Tune::Block), table_len);
    SDL_RWclose(file);
    return true;
}


bool load_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (!file) return false;
    Tune& t = player::tune();
    SDL_RWread(file, &t.tempo, sizeof(int), 1);
    SDL_RWread(file, &t.swing, sizeof(int), 1);
    SDL_RWread(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    int table_len;
    SDL_RWread(file, &table_len, sizeof(int), 1);
    t.table.resize(table_len);
    SDL_RWread(file, t.table.data(), sizeof(Tune::Block), table_len);
    SDL_RWclose(file);
    return true;
}


void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
}


struct Edit {
    // track view
    int track          = 1;
    int clavier_offset = 48;
    int track_page     = 0;
    int instrument     = 1;
    int effect         = 2;

    struct TrackSelect {
        bool active;
        bool allow_nil;
        int* value;
    } track_select;

} m_edit;



void song_view();
void track_view();
void (*m_view)(void);


bool track_select() {
    if (!m_edit.track_select.active) return false;

    gfx::font(FONT_SMALL);
    int track_nr = *m_edit.track_select.value;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            if (j > 0) gui::same_line();
            gui::min_item_size({ 65, 65 });
            int n = i * 16 + j;
            if (m_edit.track_select.allow_nil == false && n == 0) {
                gui::text("");
                continue;
            }

            char str[3] = "..";
            if (n > 0) sprintf(str, "%02X", n);
            if (gui::button(str, n == track_nr)) {
                *m_edit.track_select.value = n;
                m_edit.track_select.active = false;
            }
        }
    }

    return true;
}


void song_view() {

    Tune& tune = player::tune();

    gfx::font(FONT_MONO);
    for (int i = 0; i < 16; ++i) {
        gui::min_item_size({ 88, 65 });
        gui::text("%02X", i);

        if (i >= tune.table.size()) continue;
        Tune::Block& block = tune.table[i];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {

            gui::same_line();

            char str[3] = "..";
            if (block[c] > 0) sprintf(str, "%02X", block[c]);
            gui::min_item_size({ 0, 65 });
            if (gui::button(str)) {
                m_edit.track_select.active = true;
                m_edit.track_select.value = &block[c];
                m_edit.track_select.allow_nil = true;
            }
        }
    }


    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ 260, 65 });
    if (gui::button("save")) save_tune("tune");
    gui::same_line();
    gui::min_item_size({ 260, 65 });
    if (gui::button("load")) load_tune("tune");
}


void track_view() {
    enum { PAGE_LENGTH = 16 };

    // track
    gfx::font(FONT_MONO);
    gui::min_item_size({ 65, 65 });
    if (gui::button("-")) m_edit.track = std::max(1, m_edit.track - 1);
    char str[3];
    sprintf(str, "%02X", m_edit.track);
    gui::min_item_size({ 88, 65 });
    gui::same_line();
    if (gui::button(str)) {
        m_edit.track_select.active = true;
        m_edit.track_select.value = &m_edit.track;
        m_edit.track_select.allow_nil = false;
    }
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button("+")) m_edit.track = std::max(1, m_edit.track + 1);

    // track pages
    gui::same_line();
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
    gui::drag_int("page", m_edit.track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);

    // clavier slider
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    enum { COLS = 21 };
    gui::drag_int("clavier", m_edit.clavier_offset, 0, 96 - COLS, COLS);


    int player_row = player::row();
    assert(m_edit.track > 0);
    Track& track = player::tune().tracks[m_edit.track - 1];

    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int row_nr = m_edit.track_page * PAGE_LENGTH + i;
        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4] = ".";

        // instrument
        if (row.instrument > 0) str[0] = '0' - 1  + row.instrument;
        gui::min_item_size({ 0, 65 });
        if (gui::button(str, highlight)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = m_edit.instrument;
        }

        // effect
        str[0] = '.';
        if (row.effect > 0) str[0] = '0' - 1  + row.effect;
        gui::same_line();
        gui::min_item_size({ 0, 65 });
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
        gui::min_item_size({ 0, 65 });
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
}


} // namespace


bool init() {
    m_pref_path = SDL_GetPrefPath("sdl", "rausch");
    if (!m_pref_path) return false;

    // default tune
    Tune& t = player::tune();
    t.tempo = 5;
    t.table.resize(8);
    t.table[0] = { 1, 0, 0, 0 };
    Track& track = t.tracks[0];
    track.rows[0] = { 0, 0, 37 };
    track.rows[2] = { 0, 0, -1 };
    track.rows[4] = { 0, 0, 49 };
    track.rows[6] = { 0, 0, -1 };

    // try to load last tune
//    load_tune("tune");

    m_view = song_view;

	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
	SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);
    return true;
}


void free() {
//    save_tune("tune");
    SDL_free(m_pref_path);
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
    gfx::font(FONT_SMALL);
    gui::text("%63d", tick++);
    gui::begin_frame();


    if (!track_select()) {
        gfx::font(FONT_DEFAULT);

        gui::min_item_size({ 260, 65 });
        if (gui::button("song view", m_view == song_view)) m_view = song_view;
        gui::same_line();
        gui::min_item_size({ 260, 65 });
        if (gui::button("track view", m_view == track_view)) m_view = track_view;

        m_view();

        gfx::font(FONT_DEFAULT);
        gui::min_item_size({ 260, 0 });
        if (gui::button("play", player::is_playing())) player::play();
        gui::same_line();
        gui::min_item_size({ 260, 0 });
        if (gui::button("pause")) player::pause();
        gui::same_line();
        gui::min_item_size({ 260, 0 });
        if (gui::button("stop")) player::stop();
    }


    gfx::present();
}


} // namespace
