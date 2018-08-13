#include "project_edit.hpp"
#include "edit.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "wavelog.hpp"
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>


#define FILE_SUFFIX ".sng"

namespace {

int                      m_file_scroll;
std::string              m_dir_name = ".";
std::array<char, 25>     m_file_name;
std::vector<std::string> m_file_names;



bool init() {

#ifdef __ANDROID__
    m_dir_name = SDL_AndroidGetExternalStoragePath();
#endif

    m_dir_name += "/songs/";
    struct stat st = {};
    if (stat(m_dir_name.c_str(), &st) == -1) {
        mkdir(m_dir_name.c_str(), 0700);
    }

    return true;
}


} // namespace



void init_file_names() {

    static bool init_done = false;
    if (!init_done) {
        init();
        init_done = true;
    }

    m_file_names.clear();
    if (DIR* dir = opendir(m_dir_name.c_str())) {
        while (struct dirent* ent = readdir(dir)) {
            if (ent->d_type == DT_REG) {
                std::string name = ent->d_name;
                if (name.size() > 4 && name.substr(name.size() - 4) == FILE_SUFFIX) {
                    m_file_names.emplace_back(name.substr(0, name.size() - 4));
                }
            }
        }
        closedir(dir);
        std::sort(m_file_names.begin(), m_file_names.end());
    }
}


void draw_project_view() {

    enum {
        PAGE_LENGTH = 8
    };

    // file select
    int max_scroll = std::max<int>(PAGE_LENGTH, m_file_names.size()) - PAGE_LENGTH;
    if (m_file_scroll > max_scroll) m_file_scroll = max_scroll;

    gui::same_line();
    Vec c1 = gui::cursor() + Vec(-65 - gui::PADDING, + gui::PADDING + gui::SEPARATOR_WIDTH);
    gui::next_line();

    auto widths = calculate_column_widths({ -1, gui::SEPARATOR_WIDTH, 65 });
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int nr = i + m_file_scroll;
        gui::min_item_size({ widths[0], 65 });
        if (nr < (int) m_file_names.size()) {
            bool select = strcmp(m_file_names[nr].c_str(), m_file_name.data()) == 0;
            if (gui::button(m_file_names[nr].c_str(), select)) {
                strcpy(m_file_name.data(), m_file_names[nr].c_str());
            }
        }
        else {
            gui::padding({});
        }
        gui::same_line();
        gui::separator();
        gui::padding({ widths[2], 0 });
    }

    // scrollbar
    Vec c2 = gui::cursor();
    gui::cursor(c1);
    gui::min_item_size({ 65, c2.y - c1.y - gui::PADDING });
    gui::vertical_drag_int(m_file_scroll, 0, max_scroll, PAGE_LENGTH);
    gui::cursor(c2);
    gui::separator();

    // name
    widths = calculate_column_widths({ -1 });
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(m_file_name.data(), m_file_name.size() - 1);
    gui::separator();


    // file buttons
    widths = calculate_column_widths({ -1, -1, -1 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Load")) {
        std::string name = m_file_name.data();
        if (std::find(m_file_names.begin(), m_file_names.end(), name) != m_file_names.end()) {
            std::string path = m_dir_name + name + FILE_SUFFIX;
            load_song(player::song(), path.c_str());
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("Save")) {
        std::string name = m_file_name.data();
        std::string path = m_dir_name + name + FILE_SUFFIX;
        save_song(player::song(), path.c_str());
        init_file_names();
    }
    gui::same_line();
    gui::min_item_size({ widths[2], 88 });
    if (gui::button("Delete")) {
        std::string name = m_file_name.data();
        if (std::find(m_file_names.begin(), m_file_names.end(), name) != m_file_names.end()) {
            std::string path = m_dir_name + name + FILE_SUFFIX;
            unlink(path.c_str());
            init_file_names();
        }
    }


// TODO: export in android
#ifndef __ANDROID__
    // TODO: make it incremental
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Export to ogg")) {

        // stop
        edit::set_playing(false);
        //TODO: ensure that the callback has exited

        player::reset();
        player::block(0);
        player::block_loop(false);

        Song const& song = player::song();

        int samples = SAMPLES_PER_FRAME;
        samples *= TRACK_LENGTH * song.tempo + TRACK_LENGTH / 2 * song.swing;
        samples *= song.table_length;

        static std::array<short, 1024> buffer;

        wavelog::init(MIXRATE);
        while (samples > 0) {
            int len = std::min(samples, (int) buffer.size());
            samples -= len;
            player::fill_buffer(buffer.data(), len);
            wavelog::write(buffer.data(), len);
        }
        wavelog::free();
    }
#endif

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

}



