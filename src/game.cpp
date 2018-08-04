#include "game.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "wavelog.hpp"
#include <algorithm>


namespace game {
namespace {

enum {
    PAGE_LENGTH = 16
};

char* m_pref_path;


bool save_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "wb");
    if (!file) return false;
    Tune& t = player::tune();
    SDL_WriteU8(file, t.tempo);
    SDL_WriteU8(file, t.swing);
    SDL_RWwrite(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    SDL_RWwrite(file, t.instruments.data(), sizeof(Instrument), t.instruments.size());
    SDL_RWwrite(file, t.effects.data(), sizeof(Effect), t.effects.size());
    SDL_WriteU8(file, t.table_length);
    SDL_RWwrite(file, t.table.data(), sizeof(Tune::Block), t.table_length);
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
    t.tempo = SDL_ReadU8(file);
    t.swing = SDL_ReadU8(file);
    SDL_RWread(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    SDL_RWread(file, t.instruments.data(), sizeof(Instrument), t.instruments.size());
    SDL_RWread(file, t.effects.data(), sizeof(Effect), t.effects.size());
    t.table_length = SDL_ReadU8(file);
    SDL_RWread(file, t.table.data(), sizeof(Tune::Block), t.table_length);
    SDL_RWclose(file);
    return true;
}


void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
    wavelog::write((short*) stream, len / 2);
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
        bool operator<(Entry const& rhs) const { return data < rhs.data; }
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

} m_edit;


enum EView {
    VIEW_SONG,
    VIEW_TRACK,
    VIEW_INSTRUMENT,
    VIEW_EFFECT,
};

void song_view();
void track_view();
void instrument_view();
void effect_view();

EView m_view = VIEW_SONG;

struct View {
    char const* name;
    void (*draw)(void);
};

constexpr std::array<View, 4> views = {
    View{ "song", song_view },
    View{ "track", track_view },
    View{ "instrument", instrument_view },
    View{ "effect", effect_view },
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
        "YZ_@0123456789#$*+=<>!?#"[nr];
    dst[1] = '\0';
}


bool instrument_select() {
    if (!m_edit.instrument_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("cancel")) {
        m_edit.instrument_select_active = false;
    }
    gui::separator();

    Tune& tune = player::tune();

    for (int y = 0; y < INSTRUMENT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            if (x) gui::same_line();

            int nr = y + x * (INSTRUMENT_COUNT / 2) + 1;
            Instrument const& inst = tune.instruments[nr - 1];

            Vec c = gui::cursor() + Vec(gui::PADDING);
            gui::min_item_size({ widths[x], 65 });
            if (inst.length > 0) gui::highlight();
            if (gui::button("", nr == m_edit.instrument)) {
                m_edit.instrument_select_active = false;
                m_edit.instrument = nr;
                m_edit.inst_cache.insert(m_edit.instrument);
            }
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::print(c + Vec(65 / 2) - gfx::text_size(str) / 2, str);
            char const* name = inst.name.data();
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
    if (gui::button("cancel")) {
        m_edit.effect_select_active = false;
    }
    gui::separator();

    Tune& tune = player::tune();

    for (int y = 0; y < EFFECT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            if (x) gui::same_line();

            int nr = y + x * (EFFECT_COUNT / 2) + 1;
            Effect const& effect = tune.effects[nr - 1];

            Vec c = gui::cursor() + Vec(gui::PADDING);
            gui::min_item_size({ widths[x], 65 });
            if (effect.length > 0) gui::highlight();
            if (gui::button("", nr == m_edit.effect)) {
                m_edit.effect_select_active = false;
                m_edit.effect = nr;
                m_edit.effect_cache.insert(m_edit.effect);
            }
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::print(c + Vec(65 / 2) - gfx::text_size(str) / 2, str);
            char const* name = effect.name.data();
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
    if (gui::button("cancel")) {
        m_edit.track_select.active = false;
    }
    gui::same_line();
    if (m_edit.track_select.allow_nil) {
        gui::min_item_size({ widths[1], 88 });
        if (gui::button("clear")) {
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


void song_view() {

    gfx::font(FONT_MONO);

    auto widths = calculate_column_widths({ 88, 6, -1, -1, -1, -1 });

    // mute buttons
    gui::padding({ widths[0], 65 });
    gui::same_line();
    gui::separator();
    gui::same_line();


    gfx::font(FONT_DEFAULT);
    char str[] = "voice .";
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

    Tune& tune = player::tune();
    auto& table = tune.table;

    gfx::font(FONT_MONO);
    int block_nr = player::block();
    for (int i = 0; i < 16; ++i) {
        bool highlight = i == block_nr;

        sprintf(str, "%02X", i);
        gui::min_item_size({ widths[0], 65 });
        if (highlight) gui::highlight();
        if (gui::button(str, i == m_edit.block)) {
            m_edit.block = i;
        }
        if (gui::hold()) {
            player::block(i);
        }
        gui::same_line();
        gui::separator();
        if (i >= tune.table_length) {
            gui::padding({});
            continue;
        }


        Tune::Block& block = table[i];

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


    // buttons
    gfx::font(FONT_MONO);
    widths = calculate_column_widths({ 88, 88, -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("-")) {
        if (m_edit.block < tune.table_length && tune.table_length > 1) {
            table[m_edit.block] = {};
            std::rotate(
                table.begin() + m_edit.block,
                table.begin() + m_edit.block + 1,
                table.begin() + tune.table_length);
            --tune.table_length;
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("+")) {
        if (m_edit.block <= tune.table_length && tune.table_length < MAX_SONG_LENGTH) {
            std::rotate(
                table.begin() + m_edit.block,
                table.begin() + tune.table_length,
                table.begin() + tune.table_length + 1);
            ++tune.table_length;
        }
    }
    gfx::font(FONT_DEFAULT);
    gui::same_line();
    gui::min_item_size({ widths[2], 88 });
    if (gui::button("save")) save_tune("tune");
    gui::same_line();
    gui::min_item_size({ widths[3], 88 });
    if (gui::button("load")) load_tune("tune");
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
    enum { PAGE_LENGTH = 16 };


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
    Track& track = player::tune().tracks[m_edit.track - 1];

    gui::same_line();
    gui::padding({ widths[3], 0 });

    // copy & paste
    gfx::font(FONT_DEFAULT);
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
    gui::drag_int("clavier", m_edit.clavier_offset, 0, 96 - CLAVIER_WIDTH, CLAVIER_WIDTH);

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
            m_edit.instrument = row.instrument;
            m_edit.inst_cache.insert(m_edit.instrument);
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
            m_edit.effect = row.effect;
            m_edit.effect_cache.insert(m_edit.effect);
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
    gui::drag_int("page", m_edit.track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);

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

    Tune& tune = player::tune();
    Instrument& inst = tune.instruments[m_edit.instrument - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(inst.name.data(), inst.name.size() - 1);

    // copy & paste
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
    if (gui::button("wave", !m_edit.filter_mode)) m_edit.filter_mode = false;
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("filter", m_edit.filter_mode)) m_edit.filter_mode = true;
    gui::separator();


    if (!m_edit.filter_mode) {

        // adsr
        widths = calculate_column_widths({ -1, -1 });
        for (int i = 0; i < 4; ++i) {
            if (i % 2 > 0) gui::same_line();
            gui::min_item_size({ widths[i % 2], 65 });
            gui::id(&inst.adsr[i]);
            int v = inst.adsr[i];
            if (gui::drag_int("%X", v, 0, 15)) {
                inst.adsr[i] = v;
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
                { NOISE, "\x13" },
                { PULSE, "\x14" },
                { SAW,   "\x15" },
                { TRI,   "\x16" },
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
            if (gui::drag_int("%02X", v, 0, 31)) row.value = v;
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
        char str[] = "voice .";
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
            if (gui::button("L", row.type & FILTER_LOW)) row.type ^= FILTER_LOW;
            gui::same_line();
            gui::min_item_size({ 65, 65 });
            if (gui::button("B", row.type & FILTER_BAND)) row.type ^= FILTER_BAND;
            gui::same_line();
            gui::min_item_size({ 65, 65 });
            if (gui::button("H", row.type & FILTER_HIGH)) row.type ^= FILTER_HIGH;

            // resonance
            gui::same_line();
            gui::separator();

            int v = row.resonance;
            gui::min_item_size({ 240, 65 });
            gui::id(&row.resonance);
            if (gui::drag_int("%X", v, 0, 15)) row.resonance = v;


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
            if (gui::drag_int("%02X", v, 0, 31)) row.value = v;
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


    Tune& tune = player::tune();
    Effect& effect = tune.effects[m_edit.effect - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(effect.name.data(), effect.name.size() - 1);

    // copy & paste
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
        if (gui::drag_int("%+3d", v, -16, 16)) effect.rows[i] = (effect.rows[i] & 0x03) | ((v + 32) << 2);

        v = effect.rows[i] & 0x03;
        gui::same_line();
        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
        gui::id(&effect.rows[i + MAX_EFFECT_LENGTH]);
        if (gui::drag_int("%d", v, 0, 3)) {
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


bool init() {
    m_pref_path = SDL_GetPrefPath("sdl", "rausch");
    if (!m_pref_path) return false;


    // default tune
    Tune& t = player::tune();
    t.tempo = 5;
    t.table_length = 1;
    t.table = {
        { 1, 0, 0, 0 }
    };
    Track& track = t.tracks[0];
    track.rows[0] = { 1, 1, 37 };
    track.rows[2] = { 0, 0, 255 };
    track.rows[4] = { 1, 1, 49 };
    track.rows[6] = { 0, 2, 0 };
    track.rows[14] = { 0, 0, 255 };

    Instrument& i = t.instruments[0];
    i.adsr = { 1, 8, 8, 8 };
    i.rows[0] = { NOISE | GATE, OP_SET, 0x8 };
    i.rows[1] = { PULSE | GATE, OP_INC, 0x1 };
    i.length = 2;
    i.loop = 1;
    i.filter.length = 1;
    i.filter.routing = 1;
    i.filter.rows = { FILTER_LOW, 15, OP_SET, 25 };


    // blank effect
    strcpy(t.effects[0].name.data(), "blank");

    // vibrato
    Effect& e = t.effects[1];
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

    wavelog::init(MIXRATE);
	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
	SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);
    return true;
}


void free() {
    SDL_free(m_pref_path);
    wavelog::free();
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
        bool is_playing = player::is_playing();
        bool block_loop = player::block_loop();
        widths = calculate_column_widths({ -1, -1, -1 });

        gui::min_item_size({ widths[0], 88 });
        if (gui::button("loop", block_loop)) player::block_loop(!block_loop);

        gui::same_line();
        gui::min_item_size({ widths[1], 88 });
        if (gui::button("\x11")) player::stop();

        gui::same_line();
        gui::min_item_size({ widths[2], 88 });
        if (gui::button("\x10\x12", is_playing)) {
            if (is_playing) player::pause();
            else player::play();
        }

    }

    gfx::present();
}


} // namespace
