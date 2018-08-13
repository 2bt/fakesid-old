#include "edit.hpp"
#include "project_edit.hpp"
#include "song_edit.hpp"
#include "track_edit.hpp"
#include "gui.hpp"
#include "player.hpp"
#include <algorithm>



// android test
//void test();
//extern std::string test_path;


namespace edit {
namespace {


void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
}


EView      m_view;
bool       m_filter_mode;
bool       m_instrument_select_active;
bool       m_effect_select_active;
bool       m_is_playing;

Instrument m_copy_inst;
Effect     m_copy_effect;


bool draw_instrument_select() {
    if (!m_instrument_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_instrument_select_active = false;
    }
    gui::separator();

    Song& song = player::song();

    for (int y = 0; y < INSTRUMENT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            int nr = y + x * (INSTRUMENT_COUNT / 2) + 1;
            Instrument const& inst = song.instruments[nr - 1];

            Vec c1 = gui::cursor();
            gui::min_item_size({ widths[x], 65 });
            if (inst.length > 0) gui::highlight();
            if (gui::button("", nr == selected_instrument())) {
                m_instrument_select_active = false;
                select_instrument(nr);
            }

            gui::same_line();
            Vec c2 = gui::cursor();
            gui::cursor(c1);
            gui::min_item_size({ 65, 65 });
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gui::text(str);
            gfx::font(FONT_DEFAULT);
            gui::same_line();
            gui::min_item_size({ widths[x] - 65 - gui::PADDING, 65 });
            gui::text(inst.name.data());
            gui::same_line();
            gui::cursor(c2);
        }
        gui::next_line();
    }
    gui::separator();

    return true;
}
// XXX: this is a copy of instrument_select with s/instrument/effect/g :(
bool draw_effect_select() {
    if (!m_effect_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_effect_select_active = false;
    }
    gui::separator();

    Song& song = player::song();

    for (int y = 0; y < EFFECT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            int nr = y + x * (EFFECT_COUNT / 2) + 1;
            Effect const& effect = song.effects[nr - 1];

            Vec c1 = gui::cursor();
            gui::min_item_size({ widths[x], 65 });
            if (effect.length > 0) gui::highlight();
            if (gui::button("", nr == selected_effect())) {
                m_effect_select_active = false;
                select_effect(nr);
            }

            gui::same_line();
            Vec c2 = gui::cursor();
            gui::cursor(c1);
            gui::min_item_size({ 65, 65 });
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gui::text(str);
            gfx::font(FONT_DEFAULT);
            gui::same_line();
            gui::min_item_size({ widths[x] - 65 - gui::PADDING, 65 });
            gui::text(effect.name.data());
            gui::same_line();
            gui::cursor(c2);
        }
        gui::next_line();
    }
    gui::separator();

    return true;
}


enum {
    PAGE_LENGTH = 16
};


void draw_instrument_view() {

    // cache
    draw_instrument_cache();
    gui::separator();

    Song& song = player::song();
    Instrument& inst = song.instruments[selected_instrument() - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(inst.name.data(), inst.name.size() - 1);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_copy_inst = inst;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) inst = m_copy_inst;
    gui::separator();


    // wave/filter switch
    gfx::font(FONT_DEFAULT);
    widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Wave", !m_filter_mode)) m_filter_mode = false;
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("Filter", m_filter_mode)) m_filter_mode = true;
    gui::separator();


    if (!m_filter_mode) {

        // adsr
        constexpr char const* labels[] = { "Attack", "Decay", "Sustain", "Release" };
        widths = calculate_column_widths({ -1, -1, 88 });
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], 65 });
            gui::drag_int(labels[i], "%X", inst.adsr[i], 0, 15);
        };
        Vec c = gui::cursor();
        gui::same_line();
        gui::min_item_size({ 88, 65 * 2 + gui::PADDING });
        if (gui::button("\x11", inst.hard_restart)) inst.hard_restart ^= 1;
        gui::cursor(c);
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], 65 });
            gui::drag_int(labels[i + 2], "%X", inst.adsr[i + 2], 0, 15);
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
            gui::drag_int("", "%02X", row.value, 0, 31);
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

            gui::min_item_size({ 240, 65 });
            gui::drag_int("", "%X", row.resonance, 0, 15);


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
            gui::drag_int("", "%02X", row.value, 0, 31);
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
    gui::separator();
}


void draw_effect_view() {

    // cache
    draw_effect_cache();
    gui::separator();


    Song& song = player::song();
    Effect& effect = song.effects[selected_effect() - 1];

    // name
    auto widths = calculate_column_widths({ -1, 88, 88 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(effect.name.data(), effect.name.size() - 1);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_copy_effect = effect;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) effect = m_copy_effect;

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
    gui::separator();
}


} // namespace


bool is_playing() { return m_is_playing; }

void set_playing(bool p) {
    if (p == m_is_playing) return;
    SDL_PauseAudio(!p);
    m_is_playing = p;
}

void set_view(EView v) {
    m_view = v;
    if (m_view == VIEW_PROJECT) init_file_names();
}


bool init() {

//    test();

    set_view(VIEW_PROJECT);

    init_song(player::song());

    SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
    SDL_OpenAudio(&spec, nullptr);
    return true;
}


void free() {
    SDL_CloseAudio();
}

void draw() {
    gfx::clear();
    gui::begin_frame();

    if (!draw_track_select() && !draw_instrument_select() && !draw_effect_select()) {
        gfx::font(FONT_DEFAULT);

        // view select buttons
        struct View {
            char const* name;
            void (*draw)(void);
        };
        constexpr std::array<View, 5> views = {
            View{ "Project", draw_project_view },
            View{ "Song", draw_song_view },
            View{ "Track", draw_track_view },
            View{ "Instrum.", draw_instrument_view },
            View{ "Effect", draw_effect_view },
        };

        auto widths = calculate_column_widths(std::vector<int>(views.size(), -1));
        for (int i = 0; i < (int) views.size(); ++i) {
            if (i) gui::same_line();
            gui::min_item_size({ widths[i], 88 });
            bool button = gui::button(views[i].name, m_view == i);
            bool hold = gui::hold();
            if (button || hold) {
                if (m_view == i || hold) {
                    // open select menu
                    switch (i) {
                    case VIEW_TRACK:
                        enter_track_select();
                        break;
                    case VIEW_INSTRUMENT:
                        m_instrument_select_active = true;
                        break;
                    case VIEW_EFFECT:
                        m_effect_select_active = true;
                        break;
                    default: break;
                    }
                }
                else {
                    set_view((EView) i);
                }
            }
        }
        gui::separator();

        views[m_view].draw();

        gui::cursor({ 0, gfx::screensize().y  - gui::PADDING * 2 - 88 });
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
            set_playing(false);
            player::reset();
            player::block(get_selected_block());
        }

        // play/pause
        gui::same_line();
        gui::min_item_size({ widths[2], 88 });
        if (gui::button("\x10\x12", is_playing())) {
            set_playing(!is_playing());
        }

    }

    gfx::present();
}


} // namespace
