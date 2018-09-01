#include "edit.hpp"
#include "project_view.hpp"
#include "song_view.hpp"
#include "track_view.hpp"
#include "jam_view.hpp"
#include "instrument_effect_view.hpp"
#include "help_view.hpp"
#include "gui.hpp"
#include "player.hpp"


namespace edit {
namespace {

void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
}

EView m_view;
bool  m_is_playing;
void (*m_popup_func)(void);


} // namespace


bool is_playing() { return m_is_playing; }

void set_playing(bool p) {
    if (p == m_is_playing) return;
    SDL_PauseAudio(!p);
    m_is_playing = p;
}

void set_view(EView v) {
    m_view = v;
    if (m_view == VIEW_PROJECT) init_project_view();
}


void set_popup(void (*func)(void)) {
    m_popup_func = func;
}


bool init() {
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

    // view select buttons
    struct View {
        char const* name;
        void (*draw)(void);
    };
    constexpr std::array<View, 7> views = {
        View{ "Project", draw_project_view },
        View{ "Song", draw_song_view },
        View{ "Track", draw_track_view },
        View{ "Instr.", draw_instrument_view },
        View{ "Effect", draw_effect_view },
        View{ "Jam", draw_jam_view },
        View{ "?", draw_help_view },
    };
    std::vector<int> weights = std::vector<int>(views.size() - 1, -1);
    weights.push_back(65);
    auto widths = calculate_column_widths(weights);

    if (m_popup_func) m_popup_func();
    else {

        gfx::font(FONT_DEFAULT);
        for (int i = 0; i < (int) views.size(); ++i) {
            if (i) gui::same_line();
            gui::min_item_size({ widths[i], 88 });
            bool button = gui::button(views[i].name, m_view == i);
            bool hold   = (i == VIEW_INSTRUMENT || i == VIEW_EFFECT) && gui::hold();
            if (button || hold) {
                if (m_view == i || hold) {
                    // open select menu
                    switch (i) {
                    case VIEW_TRACK:
                        enter_track_select();
                        break;
                    case VIEW_INSTRUMENT:
                        enter_instrument_select();
                        break;
                    case VIEW_EFFECT:
                        enter_effect_select();
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
