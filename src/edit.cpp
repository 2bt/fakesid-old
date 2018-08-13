#include "edit.hpp"
#include "project_edit.hpp"
#include "song_edit.hpp"
#include "track_edit.hpp"
#include "instrument_effect_edit.hpp"
#include "gui.hpp"
#include "player.hpp"



// android test
//void test();
//extern std::string test_path;

namespace edit {
namespace {

void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
}

EView m_view;
bool  m_is_playing;


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
