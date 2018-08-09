#include "game.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "wavelog.hpp"
#include <vector>
#include <algorithm>


namespace game {
namespace {

enum {
    PAGE_LENGTH = 16
};

char* m_pref_path;


bool save_song(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "wb");
    if (!file) return false;
    Song& s = player::song();
    SDL_WriteU8(file, s.tempo);
    SDL_WriteU8(file, s.swing);
    SDL_RWwrite(file, s.tracks.data(), sizeof(Track), s.tracks.size());
    SDL_RWwrite(file, s.instruments.data(), sizeof(Instrument), s.instruments.size());
    SDL_RWwrite(file, s.effects.data(), sizeof(Effect), s.effects.size());
    SDL_WriteLE16(file, s.table_length);
    SDL_RWwrite(file, s.table.data(), sizeof(Song::Block), s.table_length);
    SDL_RWclose(file);
    return true;
}


bool load_song(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (!file) return false;
    Song& s = player::song();
    s.tempo = SDL_ReadU8(file);
    s.swing = SDL_ReadU8(file);
    SDL_RWread(file, s.tracks.data(), sizeof(Track), s.tracks.size());
    SDL_RWread(file, s.instruments.data(), sizeof(Instrument), s.instruments.size());
    SDL_RWread(file, s.effects.data(), sizeof(Effect), s.effects.size());
    s.table_length = SDL_ReadLE16(file);
    SDL_RWread(file, s.table.data(), sizeof(Song::Block), s.table_length);
    SDL_RWclose(file);
    return true;
}


void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
}


std::vector<int> calculate_column_widths(std::vector<int> weights) {
    int absolute = gfx::screensize().x - gui::PADDING;
    int relative = 0;
    for (int w : weights) {
        if (w > 0) {
            absolute -= w + gui::PADDING;
        }
        else {
            absolute -= gui::PADDING;
            relative += -w;
        }
    }
    std::vector<int> widths;
    widths.reserve(weights.size());
    for (int w : weights) {
        if (w > 0) {
            widths.emplace_back(w);
        }
        else {
            int q = absolute * -w / relative;
            absolute -= q;
            relative += w;
            widths.emplace_back(q);
        }
    }
    return widths;
}


struct Cache {
    Cache() {
        for (int i = 0; i < SIZE; ++i) {
            entries[i].age  = i;
            entries[i].data = i + 1;
        }
    }
    enum { SIZE = 12 };
    struct Entry {
        uint32_t age;
        int      data;
//        bool operator<(Entry const& rhs) const { return data < rhs.data; }
    };
    std::array<Entry, SIZE> entries;
    void insert(int data) {
        Entry* f = &entries[0];
        for (Entry& e : entries) {
            if (e.data == data) e.age = -1;
            else ++e.age;
            if (e.age > f->age) {
                f = &e;
            }
        }
        f->data = data;
        f->age = 0;
//        std::sort(entries.begin(), entries.end());
    }
};


struct Edit {
    int        track          = 1;
    int        clavier_offset = 36;
    int        track_page     = 0;
    int        song_page      = 0;
    int        instrument     = 1;
    int        effect         = 1;
    int        block          = 0;
    bool       filter_mode    = false;


    Track      copy_track;
    Instrument copy_inst;
    Effect     copy_effect;

    Cache      inst_cache;
    Cache      effect_cache;


    struct {
        bool active;
        bool allow_nil;
        int* value;
    } track_select;


    bool instrument_select_active;
    bool effect_select_active;

    bool is_playing = false;

} m_edit;


enum EView {
    VIEW_PROJECT,
    VIEW_SONG,
    VIEW_TRACK,
    VIEW_INSTRUMENT,
    VIEW_EFFECT,
};

void project_view();
void song_view();
void track_view();
void instrument_view();
void effect_view();

EView m_view = VIEW_PROJECT;

struct View {
    char const* name;
    void (*draw)(void);
};

constexpr std::array<View, 5> views = {
    View{ "Project", project_view },
    View{ "Song", song_view },
    View{ "Track", track_view },
    View{ "Instrum.", instrument_view },
    View{ "Effect", effect_view },
};



void sprint_track_id(char* dst, int nr) {
    if (nr == 0) {
        dst[0] = dst[1] == ' ';
    }
    else {
        int x = (nr - 1) / 21;
        int y = (nr - 1) % 21;
        dst[0] = '0' + x + (x > 9) * 7;
        dst[1] = '0' + y + (y > 9) * 7;
    }
    dst[2] = '\0';
}

void sprint_inst_effect_id(char* dst, int nr) {
    dst[0] = " "
        "ABCDEFGHIJKLMNOPQRSTUVWX"
        "YZ@0123456789#$*+=<>!?#~"[nr];
    dst[1] = '\0';
}

void set_instrument(int i) {
    m_edit.instrument = i;
    m_edit.inst_cache.insert(i);
}


void set_effect(int e) {
    m_edit.effect = e;
    m_edit.effect_cache.insert(e);
}


bool instrument_select() {
    if (!m_edit.instrument_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_edit.instrument_select_active = false;
    }
    gui::separator();

    Song& song = player::song();

    for (int y = 0; y < INSTRUMENT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            if (x) gui::same_line();

            int nr = y + x * (INSTRUMENT_COUNT / 2) + 1;
            Instrument const& inst = song.instruments[nr - 1];

            Vec c = gui::cursor() + Vec(gui::PADDING);
            gui::min_item_size({ widths[x], 65 });
            if (inst.length > 0) gui::highlight();
            if (gui::button("", nr == m_edit.instrument)) {
                m_edit.instrument_select_active = false;
                set_instrument(nr);
            }
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gfx::print(c + Vec(65 / 2) - gfx::text_size(str) / 2, str);
            char const* name = inst.name.data();
            gfx::font(FONT_DEFAULT);
            gfx::print(c + Vec(65 + (widths[x] - 65) / 2, 65 / 2) - gfx::text_size(name) / 2, name);
        }
    }
    gui::separator();

    return true;
}
// XXX: this is a copy of instrument_select with s/instrument/effect/g :(
bool effect_select() {
    if (!m_edit.effect_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_edit.effect_select_active = false;
    }
    gui::separator();

    Song& song = player::song();

    for (int y = 0; y < EFFECT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            if (x) gui::same_line();

            int nr = y + x * (EFFECT_COUNT / 2) + 1;
            Effect const& effect = song.effects[nr - 1];

            Vec c = gui::cursor() + Vec(gui::PADDING);
            gui::min_item_size({ widths[x], 65 });
            if (effect.length > 0) gui::highlight();
            if (gui::button("", nr == m_edit.effect)) {
                m_edit.effect_select_active = false;
                set_effect(nr);
            }
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gfx::print(c + Vec(65 / 2) - gfx::text_size(str) / 2, str);
            char const* name = effect.name.data();
            gfx::font(FONT_DEFAULT);
            gfx::print(c + Vec(65 + (widths[x] - 65) / 2, 65 / 2) - gfx::text_size(name) / 2, name);
        }
    }
    gui::separator();

    return true;
}


bool track_select() {
    if (!m_edit.track_select.active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_edit.track_select.active = false;
    }
    gui::same_line();
    if (m_edit.track_select.allow_nil) {
        gui::min_item_size({ widths[1], 88 });
        if (gui::button("Clear")) {
            m_edit.track_select.active = false;
            *m_edit.track_select.value = 0;
        }
    }
    else {
        gui::padding({});
    }

    gui::separator();

    gfx::font(FONT_MONO);
    widths = calculate_column_widths(std::vector<int>(12, -1));
    int track_nr = *m_edit.track_select.value;

    for (int y = 0; y < 21; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (x > 0) gui::same_line();
            int n = x * 21 + y + 1;

            gui::min_item_size({ widths[x], 65 });
            char str[3] = "  ";
            sprint_track_id(str, n);
            if (gui::button(str, n == track_nr)) {
                *m_edit.track_select.value = n;
                m_edit.track_select.active = false;
            }
        }
    }

    return true;
}


void project_view() {

    auto widths = calculate_column_widths({ -1, -1 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Load")) load_song("song");
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("Save")) save_song("song");


    // TODO: make it incremental
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Export to ogg")) {

        // stop
        m_edit.is_playing = false;
        SDL_PauseAudio(1);
        //TODO: ensure that the callback has exited

        player::reset();
        player::block(0);
        player::block_loop(false);


        Song const& song = player::song();
        int samples = SAMPLES_PER_FRAME;
        samples *= TRACK_LENGTH * song.tempo + TRACK_LENGTH / 2 * song.swing;
        samples *= song.table_length;

        std::array<short, 1024> buffer;

        wavelog::init(MIXRATE);
        while (samples > 0) {
            int len = std::min(samples, (int) buffer.size());
            samples -= len;
            player::fill_buffer(buffer.data(), len);
            wavelog::write(buffer.data(), len);
        }
        wavelog::free();
    }

}


void song_view() {
    Song& song = player::song();


    // tempo and swing
    auto widths = calculate_column_widths({ -9, -5 });
    int v = song.tempo;
    gui::min_item_size({ widths[0], 0 });
    if (gui::drag_int("Tempo", "%X", v, 4, 12)) song.tempo = v;
    gui::same_line();
    v = song.swing;
    gui::min_item_size({ widths[1], 0 });
    if (gui::drag_int("Swing", "%X", v, 0, 4)) song.swing = v;
    gui::separator();

    // mute buttons
    widths = calculate_column_widths({ 88, 6, -1, -1, -1, -1 });
    gui::padding({ widths[0], 65 });
    gui::same_line();
    gui::separator();
    gui::same_line();
    gfx::font(FONT_DEFAULT);
    char str[] = "Voice .";
    for (int c = 0; c < CHANNEL_COUNT; ++c) {
        str[6] = '1' + c;
        gui::same_line();
        gui::min_item_size({ widths[c + 2], 65 });
        bool a = player::is_channel_active(c);
        if (gui::button(str, a)) {
            player::set_channel_active(c, !a);
        }
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    auto& table = song.table;

    gfx::font(FONT_MONO);
    int player_block = player::block();
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int block_nr = m_edit.song_page * PAGE_LENGTH + i;
        bool highlight = block_nr == player_block;

        sprintf(str, "%02X", block_nr);
        gui::min_item_size({ widths[0], 65 });
        if (highlight) gui::highlight();
        if (gui::button(str, block_nr == m_edit.block)) {
            m_edit.block = block_nr;
        }
        gui::same_line();
        gui::separator();
        if (block_nr >= song.table_length) {
            gui::padding({});
            continue;
        }


        Song::Block& block = table[block_nr];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {

            gui::same_line();

            char str[3];
            sprint_track_id(str, block[c]);
            gui::min_item_size({ widths[c + 2], 65 });
            if (highlight) gui::highlight();
            if (gui::button(str)) {
                m_edit.track_select.active = true;
                m_edit.track_select.value = &block[c];
                m_edit.track_select.allow_nil = true;
            }
            if (block[c] > 0 && gui::hold()) {
                gui::block_touch();
                m_edit.track = block[c];
                m_view = VIEW_TRACK;
            }
        }
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    // song pages
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    gui::drag_int("Page", "%X", m_edit.song_page, 0, (song.table_length + PAGE_LENGTH - 1) / PAGE_LENGTH - 1);
    gui::separator();



    // buttons
    gfx::font(FONT_MONO);
    gui::min_item_size({ 88, 88 });
    if (gui::button("-")) {
        if (m_edit.block < song.table_length && song.table_length > 1) {
            table[m_edit.block] = {};
            std::rotate(
                table.begin() + m_edit.block,
                table.begin() + m_edit.block + 1,
                table.begin() + song.table_length);
            --song.table_length;
        }
    }
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("+")) {
        if (m_edit.block <= song.table_length && song.table_length < MAX_SONG_LENGTH) {
            std::rotate(
                table.begin() + m_edit.block,
                table.begin() + song.table_length,
                table.begin() + song.table_length + 1);
            ++song.table_length;
        }
    }
}


void draw_cache(Cache& cache, int& data) {
    gfx::font(FONT_MONO);
    auto widths = calculate_column_widths(std::vector<int>(Cache::SIZE, -1));
    for (int i = 0; i < Cache::SIZE; ++i) {
        if (i) gui::same_line();
        auto const& e = cache.entries[i];
        char str[2];
        sprint_inst_effect_id(str, e.data);
        gui::min_item_size({ widths[i], 88 });
        if (gui::button(str, e.data == data)) {
            data = e.data;
            cache.insert(data);
        }
    }
}

void inst_cache() {
    draw_cache(m_edit.inst_cache, m_edit.instrument);
}

void effect_cache() {
    draw_cache(m_edit.effect_cache, m_edit.effect);
}

void track_view() {

    auto widths = calculate_column_widths({ 88, 88, 88, -1, 88, 88 });

    // track select
    gfx::font(FONT_MONO);
    gui::min_item_size({ 88, 88 });
    if (gui::button("-")) m_edit.track = std::max(1, m_edit.track - 1);
    char str[3];
    sprint_track_id(str, m_edit.track);
    gui::min_item_size({ 88, 88 });
    gui::same_line();
    if (gui::button(str)) {
        m_edit.track_select.active = true;
        m_edit.track_select.value = &m_edit.track;
        m_edit.track_select.allow_nil = false;
    }
    gui::min_item_size({ 88, 88 });
    gui::same_line();
    if (gui::button("+")) m_edit.track = std::min<int>(TRACK_COUNT, m_edit.track + 1);

    assert(m_edit.track > 0);
    Track& track = player::song().tracks[m_edit.track - 1];

    gui::same_line();
    gui::padding({ widths[3], 0 });

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_edit.copy_track = track;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) track = m_edit.copy_track;



    // clavier slider
    gui::separator();
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    gui::drag_int("Clavier", "", m_edit.clavier_offset, 0, 96 - CLAVIER_WIDTH, CLAVIER_WIDTH);

    int player_row = player::row();

    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        if (i % 4 == 0) gui::separator();

        int row_nr = m_edit.track_page * PAGE_LENGTH + i;
        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4];

        // instrument
        sprint_inst_effect_id(str, row.instrument);
        gui::min_item_size({ 65, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = m_edit.instrument;
        }
        if (row.instrument > 0 && gui::hold()) {
            set_instrument(row.instrument);
        }

        // effect
        sprint_inst_effect_id(str, row.effect);
        gui::same_line();
        gui::min_item_size({ 65, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.effect > 0) row.effect = 0;
            else row.effect = m_edit.effect;
        }
        if (row.effect > 0 && gui::hold()) {
            set_effect(row.effect);
        }


        // note
        str[0] = str[1] = str[2] = ' ';
        if (row.note == 255) {
            str[0] = str[1] = str[2] = '-';
        }
        else if (row.note > 0) {
            str[0] = "CCDDEFFGGAAB"[(row.note - 1) % 12];
            str[1] = "-#-#--#-#-#-"[(row.note - 1) % 12];
            str[2] = '0' + (row.note - 1) / 12;
        }
        str[3] = '\0';
        gui::min_item_size({ 0, 65 });
        gui::same_line();
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.note == 0) row.note = 255;
            else {
                row = {};
            }
        }

        // clavier
        gui::same_line();
        gui::separator();
        if (gui::clavier(row.note, m_edit.clavier_offset, highlight)) {
            if (row.note == 0) row = {};
            else {
                row.instrument = m_edit.instrument;
                row.effect     = m_edit.effect;
            }
        }
    }
    gui::separator();

    // track pages
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    gui::drag_int("Page", "%X", m_edit.track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);

    // cache
    gui::separator();
    inst_cache();
    gui::separator();
    effect_cache();
}


void instrument_view() {

    // cache
    inst_cache();
    gui::separator();

    Song& song = player::song();
    Instrument& inst = song.instruments[m_edit.instrument - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(inst.name.data(), inst.name.size() - 1);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_edit.copy_inst = inst;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) inst = m_edit.copy_inst;
    gui::separator();



    // wave/filter switch
    gfx::font(FONT_DEFAULT);
    widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Wave", !m_edit.filter_mode)) m_edit.filter_mode = false;
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("Filter", m_edit.filter_mode)) m_edit.filter_mode = true;
    gui::separator();


    if (!m_edit.filter_mode) {

        // adsr
        constexpr char const* labels[] = { "Attack", "Decay", "Sustain", "Release" };
        widths = calculate_column_widths({ -1, -1, 88 });
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], 65 });
            gui::id(&inst.adsr[i]);
            int v = inst.adsr[i];
            if (gui::drag_int(labels[i], "%X", v, 0, 15)) {
                inst.adsr[i] = v;
            }
        };
        Vec c = gui::cursor();
        gui::same_line();
        gui::min_item_size({ 88, 65 * 2 + gui::PADDING });
        if (gui::button("\x11", inst.hard_restart)) inst.hard_restart ^= 1;
        gui::cursor(c);
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], 65 });
            gui::id(&inst.adsr[i + 2]);
            int v = inst.adsr[i + 2];
            if (gui::drag_int(labels[i + 2], "%X", v, 0, 15)) {
                inst.adsr[i + 2] = v;
            }
        };



        gui::separator();
        gfx::font(FONT_MONO);

        // wave
        for (int i = 0; i < MAX_INSTRUMENT_LENGTH; ++i) {

            char str[2];
            sprintf(str, "%X", i);
            gui::min_item_size({ 65, 65 });

            if (gui::button(str, i == inst.loop)) inst.loop = i;
            gui::same_line();
            gui::separator();
            if (i >= inst.length) {
                gui::padding({ 467, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({});
                continue;
            }
            auto& row = inst.rows[i];

            // flags
            constexpr std::pair<uint8_t, char const*> flags[] = {
                { NOISE, "\x14" },
                { PULSE, "\x15" },
                { SAW,   "\x16" },
                { TRI,   "\x17" },
                { RING,  "R" },
                { SYNC,  "S" },
                { GATE,  "G" },
            };
            for (auto p : flags) {
                gui::same_line();
                gui::min_item_size({ 65, 65 });
                if (gui::button(p.second, row.flags & p.first)) {
                    row.flags ^= p.first;
                }
            }

            gui::same_line();
            gui::separator();
            str[0] = "+=-"[row.operation];
            str[1] = '\0';
            gui::min_item_size({ 65, 65 });
            if (gui::button(str)) row.operation = !row.operation;
            gui::same_line();
            gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
            int v = row.value;
            gui::id(&row.value);
            if (gui::drag_int("", "%02X", v, 0, 31)) row.value = v;
        }

        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
        gui::separator();


        gfx::font(FONT_MONO);
        gui::min_item_size({ 88, 88 });
        if (gui::button("-")) {
            if (inst.length > 0) {
                --inst.length;
            }
        }
        gui::same_line();
        gui::min_item_size({ 88, 88 });
        if (gui::button("+") && inst.length < MAX_INSTRUMENT_LENGTH) {
            inst.rows[inst.length] = { GATE, OP_INC, 0 };
            ++inst.length;
        }
    }
    else {
        // filter table
        Filter& filter = inst.filter;

        widths = calculate_column_widths({ -1, -1, -1, -1 });
        char str[] = "Voice .";
        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            str[6] = '1' + c;
            if (c) gui::same_line();
            gui::min_item_size({ widths[c], 65 });
            if (gui::button(str, filter.routing & (1 << c))) filter.routing ^= 1 << c;
        }
        gui::separator();

        gfx::font(FONT_MONO);
        for (int i = 0; i < MAX_FILTER_LENGTH; ++i) {

            sprintf(str, "%X", i);
            gui::min_item_size({ 65, 65 });

            if (gui::button(str, i == filter.loop)) filter.loop = i;
            gui::same_line();
            gui::separator();
            if (i >= filter.length) {
                gui::padding({ 199, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({ 240, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({});
                continue;
            }
            auto& row = filter.rows[i];


            // type
            gui::same_line();
            gui::min_item_size({ 65, 65 });
            if (gui::button("\x18", row.type & FILTER_LOW)) row.type ^= FILTER_LOW;
            gui::same_line();
            gui::min_item_size({ 65, 65 });
            if (gui::button("\x19", row.type & FILTER_BAND)) row.type ^= FILTER_BAND;
            gui::same_line();
            gui::min_item_size({ 65, 65 });
            if (gui::button("\x1a", row.type & FILTER_HIGH)) row.type ^= FILTER_HIGH;

            // resonance
            gui::same_line();
            gui::separator();

            int v = row.resonance;
            gui::min_item_size({ 240, 65 });
            gui::id(&row.resonance);
            if (gui::drag_int("", "%X", v, 0, 15)) row.resonance = v;


            // operation
            gui::same_line();
            gui::separator();
            str[0] = "+=-"[row.operation];
            str[1] = '\0';
            gui::min_item_size({ 65, 65 });
            if (gui::button(str)) row.operation = (row.operation + 1) % 3;

            // cutoff
            gui::same_line();
            gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
            v = row.value;
            gui::id(&row.value);
            if (gui::drag_int("", "%02X", v, 0, 31)) row.value = v;
        }

        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
        gui::separator();


        gfx::font(FONT_MONO);
        gui::min_item_size({ 88, 88 });
        if (gui::button("-")) {
            if (filter.length > 0) {
                --filter.length;
            }
        }
        gui::same_line();
        gui::min_item_size({ 88, 88 });
        if (gui::button("+") && filter.length < MAX_FILTER_LENGTH) {
            filter.rows[filter.length] = { 0, 0, OP_SET, 0 };
            ++filter.length;
        }
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });

}


void effect_view() {

    // cache
    effect_cache();
    gui::separator();


    Song& song = player::song();
    Effect& effect = song.effects[m_edit.effect - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(effect.name.data(), effect.name.size() - 1);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_edit.copy_effect = effect;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) effect = m_edit.copy_effect;

    gui::separator();

    // rows
    gfx::font(FONT_MONO);
    for (int i = 0; i < MAX_EFFECT_LENGTH; ++i) {
        char str[2];
        sprintf(str, "%X", i);
        gui::min_item_size({ 65, 65 });

        if (gui::button(str, i == effect.loop)) effect.loop = i;
        gui::same_line();
        gui::separator();
        if (i >= effect.length) {
            gui::padding({});
            continue;
        }

        int v = (effect.rows[i] >> 2) - 32;
        gui::min_item_size({ gfx::screensize().x * 4 / 5, 65 });
        gui::id(&effect.rows[i]);
        if (gui::drag_int("", "%d", v, -16, 16)) effect.rows[i] = (effect.rows[i] & 0x03) | ((v + 32) << 2);

        v = effect.rows[i] & 0x03;
        gui::same_line();
        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
        gui::id(&effect.rows[i + MAX_EFFECT_LENGTH]);
        if (gui::drag_int("", "%d", v, 0, 3)) {
            effect.rows[i] = (effect.rows[i] & 0xfc) | v;
        }
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();


    widths = calculate_column_widths({ 88, 88, -1, -1 });
    gfx::font(FONT_MONO);
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("-")) {
        if (effect.length > 0) {
            --effect.length;
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("+") && effect.length < MAX_EFFECT_LENGTH) {
        effect.rows[effect.length] = 0x80;
        ++effect.length;
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });

}


} // namespace


void init_song() {
    Song& s = player::song();

    // instruments

    // bass
    {
        Instrument& i = s.instruments[0];
        strcpy(i.name.data(), "bass");
        i.hard_restart = 1;
        i.adsr = { 1, 8, 8, 8 };
        i.rows[0] = { NOISE | GATE, OP_SET, 13 };
        i.rows[1] = { PULSE | GATE, OP_INC, 3 };
        i.length = 2;
        i.loop = 1;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 13, OP_SET, 12 };
        i.filter.rows[1] = { FILTER_LOW, 13, OP_DEC, 8 };
        i.filter.length = 2;
        i.filter.loop = 1;
    }

    // kick
    {
        Instrument& i = s.instruments[1];
        strcpy(i.name.data(), "kick");
        i.adsr = { 1, 8, 8, 8 };
        i.hard_restart = 1;
        i.rows[0] = { NOISE | GATE, OP_SET, 13 };
        i.rows[1] = { PULSE | GATE, OP_INC, 3 };
        i.length = 2;
        i.loop = 1;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 13, OP_SET, 13 };
        i.filter.rows[1] = { FILTER_LOW, 13, OP_SET, 20 };
        i.filter.rows[2] = { FILTER_LOW, 13, OP_SET, 5 };
        i.filter.length = 3;
        i.filter.loop = 2;
    }

    // snare
    {
        Instrument& i = s.instruments[2];
        strcpy(i.name.data(), "snare");
        i.hard_restart = 1;
        i.adsr = { 1, 1, 7, 9 };
        i.rows[0] = { NOISE | GATE, OP_SET, 0x8 };
        i.rows[1] = { NOISE | GATE, OP_INC, 0x0 };
        i.rows[2] = { PULSE | GATE, OP_INC, 0x0 };
        i.rows[3] = { PULSE | GATE, OP_INC, 0x0 };
        i.rows[4] = { NOISE, OP_INC, 0x0 };
        i.length = 5;
        i.loop = 4;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 13, OP_SET, 13 };
        i.filter.rows[1] = { FILTER_LOW, 13, OP_DEC, 3 };
        i.filter.length = 2;
        i.filter.loop = 1;
    }


    // effects


    {
        // bass
        Effect& e = s.effects[0];
        strcpy(e.name.data(), "bass");
        e.rows[0] = 0x80 - 4 * 12;
        e.length = 1;
        e.loop = 0;
    }
    {
        // kick
        Effect& e = s.effects[1];
        strcpy(e.name.data(), "kick");
        e.rows[0] = 0x80 + 4 * 12;
        e.rows[1] = 0x80 + 4 * 4;
        e.rows[2] = 0x80 - 4 * 4;
        e.rows[3] = 0x80 - 4 * 12;
        e.length = 4;
        e.loop = 3;
    }
    {
        // snare
        Effect& e = s.effects[2];
        strcpy(e.name.data(), "snare");
        e.rows[0] = 0x80 + 4 * 13;
        e.rows[1] = 0x80 + 4 * 13;
        e.rows[2] = 0x80 - 4 * 6;
        e.rows[3] = 0x80 - 4 * 11;
        e.rows[4] = 0x80 + 4 * 9;
        e.rows[5] = 0x80 + 4 * 13;
        e.length = 6;
        e.loop = 4;
    }


    {
        // vibrato
        Effect& e = s.effects[INSTRUMENT_COUNT - 1];
        strcpy(e.name.data(), "vibrato");
        e.rows[0] = 0x80;
        e.rows[1] = 0x81;
        e.rows[2] = 0x82;
        e.rows[3] = 0x82;
        e.rows[4] = 0x81;
        e.rows[5] = 0x80;
        e.rows[6] = 0x7f;
        e.rows[7] = 0x7e;
        e.rows[8] = 0x7e;
        e.rows[9] = 0x7f;
        e.length = 10;
        e.loop = 0;
    }


    s.tempo = 5;
    s.table_length = 1;
    s.table = { { 1, 0, 0, 0 } };
    Track& track = s.tracks[0];
    track.rows[0] = { 2, 2, 37 };
    track.rows[2] = { 1, 1, 37 };
    track.rows[4] = { 0, 0, 255 };
    track.rows[6] = { 1, 1, 37 };
    track.rows[8] = { 3, 3, 49 };
    track.rows[10] = { 1, 1, 25 };
    track.rows[12] = { 1, 1, 37 };
    track.rows[14] = { 0, 0, 255 };
    track.rows[16] = { 2, 2, 37 };
    track.rows[18] = { 0, 0, 255 };
    track.rows[24] = { 3, 3, 49 };
    track.rows[28] = { 1, 1, 35 };
}


bool init() {
    m_pref_path = SDL_GetPrefPath("sdl", "insidious");
    if (!m_pref_path) return false;

    init_song();

    SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
    SDL_OpenAudio(&spec, nullptr);
    return true;
}


void free() {
    SDL_CloseAudio();
    SDL_free(m_pref_path);
}



void draw() {
    gfx::clear();
    gui::begin_frame();

    if (!track_select() && !instrument_select() && !effect_select()) {
        gfx::font(FONT_DEFAULT);

        // view select buttons
        auto widths = calculate_column_widths(std::vector<int>(views.size(), -1));
        for (int i = 0; i < (int) views.size(); ++i) {
            if (i) gui::same_line();
            gui::min_item_size({ widths[i], 88 });
            if (gui::button(views[i].name, m_view == i)) {
                if (m_view != i) {
                    // switch view
                    m_view = (EView) i;
                }
                else {
                    // open select menu
                    switch (m_view) {
                    case VIEW_TRACK:
                        m_edit.track_select.active = true;
                        m_edit.track_select.value = &m_edit.track;
                        m_edit.track_select.allow_nil = false;
                        break;
                    case VIEW_INSTRUMENT:
                        m_edit.instrument_select_active = true;
                        break;
                    case VIEW_EFFECT:
                        m_edit.effect_select_active = true;
                        break;
                    default: break;
                    }
                }
            }
        }
        gui::separator();

        views[m_view].draw();

        // stop and play
        int y = gfx::screensize().y - gui::cursor().y - gui::PADDING * 3 - 88;
        if (y > 0) gui::padding({ 0, y });

        gfx::font(FONT_DEFAULT);
        bool block_loop = player::block_loop();
        widths = calculate_column_widths({ -1, -1, -1 });

        // loop
        gui::min_item_size({ widths[0], 88 });
        if (gui::button("\x13", block_loop)) player::block_loop(!block_loop);

        // stop
        gui::same_line();
        gui::min_item_size({ widths[1], 88 });
        if (gui::button("\x11")) {
            m_edit.is_playing = false;
            SDL_PauseAudio(1);
            player::reset();
            player::block(m_edit.block);
        }

        // play/pause
        gui::same_line();
        gui::min_item_size({ widths[2], 88 });
        if (gui::button("\x10\x12", m_edit.is_playing)) {
            SDL_PauseAudio(m_edit.is_playing);
            m_edit.is_playing ^= 1;
        }

    }

    gfx::present();
}


} // namespace
