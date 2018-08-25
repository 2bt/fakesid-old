#include "project_edit.hpp"
#include "edit.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "wavelog.hpp"
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sndfile.h>


#define FILE_SUFFIX ".sng"


// android
std::string get_root_dir();


namespace {


sf_count_t sf_vio_get_filelen(void* user_data) {
    return SDL_RWsize((SDL_RWops*) user_data);
}

sf_count_t sf_vio_seek(sf_count_t offset, int whence, void* user_data) {
    int w = whence == SEEK_CUR ? RW_SEEK_CUR
          : whence == SEEK_SET ? RW_SEEK_SET
                               : RW_SEEK_END;
    return SDL_RWseek((SDL_RWops*) user_data, offset, w);
}

sf_count_t sf_vio_read(void* ptr, sf_count_t count, void* user_data) {
    return SDL_RWread((SDL_RWops*) user_data, ptr, 1, count);
}

sf_count_t sf_vio_write(const void* ptr, sf_count_t count, void* user_data) {
    return SDL_RWwrite((SDL_RWops*) user_data, ptr, 1, count);
}

sf_count_t sf_vio_tell(void* user_data) {
    return SDL_RWtell((SDL_RWops*) user_data);
}


SDL_RWops* m_file;
SNDFILE*   m_sndfile;


bool ogg_open(char const* name) {

    m_file = SDL_RWFromFile(name, "wb");
    if (!m_file) return false;

    SF_INFO info = { 0, MIXRATE, 1, SF_FORMAT_OGG | SF_FORMAT_VORBIS };

    SF_VIRTUAL_IO vio = {
        sf_vio_get_filelen,
        sf_vio_seek,
        sf_vio_read,
        sf_vio_write,
        sf_vio_tell,
    };

    m_sndfile = sf_open_virtual(&vio, SFM_WRITE, &info, m_file);
    if (!m_sndfile) {
        SDL_RWclose(m_file);
        m_file = nullptr;
        return false;
    }

    double quality = 0.95;
    sf_command(m_sndfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(quality));

    return m_sndfile != nullptr;
}


void ogg_write(short const* buffer, int len) {
    if (m_sndfile) sf_writef_short(m_sndfile, buffer, len);
}


void ogg_close() {
    sf_close(m_sndfile);
    SDL_RWclose(m_file);
    m_sndfile = nullptr;
    m_file = nullptr;
}



int                      m_file_scroll;
std::string              m_dir_name;
std::array<char, 28>     m_file_name;
std::vector<std::string> m_file_names;
std::string              m_export_dir;
std::string              m_status_msg;
int                      m_status_age;


void status(std::string const& msg) {
    m_status_msg = msg;
    m_status_age = 0;
}


bool copy_demo_song(const char* name) {

    std::string dst_name = m_dir_name + name;
    std::string src_name = std::string("res/") + name;

    struct stat st;
    if (stat(dst_name.c_str(), &st) != -1) return true;

    SDL_RWops* src = SDL_RWFromFile(src_name.c_str(), "rb");
    if (!src) return false;

    SDL_RWops* dst = SDL_RWFromFile(dst_name.c_str(), "wb");
    if (!dst) {
        SDL_RWclose(src);
        return false;
    }

    int len = SDL_RWseek(src, 0, RW_SEEK_END);
    SDL_RWseek(src, 0, RW_SEEK_SET);
    std::vector<uint8_t> buffer(len);

    SDL_RWread(src, buffer.data(), sizeof(uint8_t), buffer.size());
    SDL_RWwrite(dst, buffer.data(), sizeof(uint8_t), buffer.size());

    SDL_RWclose(dst);
    SDL_RWclose(src);

    return true;
}


bool init_dirs() {

    if (++m_status_age > 100) m_status_msg = "";

    std::string root_dir = get_root_dir();
    if (root_dir.empty()) return false;

    m_dir_name = root_dir + "/songs/";

    struct stat st;
    if (stat(m_dir_name.c_str(), &st) == -1) mkdir(m_dir_name.c_str(), 0700);

    m_export_dir = root_dir + "/exports/";
    if (stat(m_export_dir.c_str(), &st) == -1) mkdir(m_export_dir.c_str(), 0700);

    copy_demo_song("demo1" FILE_SUFFIX);
    copy_demo_song("demo2" FILE_SUFFIX);

    return true;
}


void ogg_export() {

    // stop
    edit::set_playing(false);
    //TODO: ensure that the callback has exited

    player::reset();
    player::block(0);
    player::block_loop(false);

    std::string name = m_file_name.data();

    if (name.empty()) {
        status("Export error: empty file name");
        return;
    }

    std::string path = m_export_dir + name + ".ogg";

    if (!ogg_open(path.c_str())) {
        status("Export error: couldn't open file");
        return;
    }

    Song& song = player::song();

    // set meta info
    sf_set_string(m_sndfile, SF_STR_TITLE, song.title.data());
    sf_set_string(m_sndfile, SF_STR_ARTIST, song.author.data());

    static std::array<short, 1024> buffer;

    int frames = (song.track_length * song.tempo + song.track_length / 2 * song.swing) * song.table_length;
    int samples = frames * SAMPLES_PER_FRAME;

    while (samples > 0) {
        int len = std::min<int>(samples, buffer.size());
        samples -= len;
        player::fill_buffer(buffer.data(), len);
        ogg_write(buffer.data(), len);
    }
    ogg_close();

    player::reset();

    status("Song was exported");
}


} // namespace



void init_project_view() {

    static bool init_dirs_done = false;
    if (!init_dirs_done) {
        init_dirs();
        init_dirs_done = true;
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

    m_status_msg = "";
}


void draw_project_view() {

    Song& song = player::song();

    // title and author
    auto widths = calculate_column_widths({ 270, -1 });
    gui::align(gui::LEFT);
    gui::min_item_size({ widths[0], 88 });
    gui::text("Title");
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    gui::input_text(song.title.data(), song.title.size() - 1);
    gui::min_item_size({ widths[0], 88 });
    gui::text("Author");
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    gui::input_text(song.author.data(), song.author.size() - 1);

    // track length
    auto widths2 = calculate_column_widths({ 270, -1, -1 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::text("Track length");
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::align(gui::CENTER);
    gui::min_item_size({ widths2[1], 88 });
    if (gui::button("24", song.track_length == 24)) song.track_length = 24;
    gui::same_line();
    gui::min_item_size({ widths2[2], 88 });
    if (gui::button("32", song.track_length == 32)) song.track_length = 32;
    gui::align(gui::LEFT);

    // length
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::text("Song length");
    gui::same_line();
    gfx::font(FONT_MONO);
    gui::min_item_size({ widths[1], 88 });
    int frames = (song.track_length * song.tempo + song.track_length / 2 * song.swing) * song.table_length;
    int seconds = frames / FRAMES_PER_SECOND;
    gui::text("%d:%02d", seconds / 60, seconds % 60);

    // tempo and swing
    widths = calculate_column_widths({ -9, -5 });
    gui::min_item_size({ widths[0], 65 });
    gui::drag_int("Tempo", "%X", song.tempo, 4, 12);
    gui::same_line();
    gui::min_item_size({ widths[1], 65 });
    gui::drag_int("Swing", "%X", song.swing, 0, 4);
    gui::separator();


    // name
    widths = calculate_column_widths({ -1 });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], 88 });
    gui::input_text(m_file_name.data(), m_file_name.size() - 1);
    gui::separator();

    // file select
    enum { PAGE_LENGTH = 10 };
    int max_scroll = std::max<int>(PAGE_LENGTH, m_file_names.size()) - PAGE_LENGTH;
    if (m_file_scroll > max_scroll) m_file_scroll = max_scroll;
    gui::same_line();
    Vec c1 = gui::cursor() + Vec(-65 - gui::PADDING, + gui::PADDING + gui::SEPARATOR_WIDTH);
    gui::next_line();
    gfx::font(FONT_DEFAULT);
    widths = calculate_column_widths({ -1, gui::SEPARATOR_WIDTH, 65 });
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int nr = i + m_file_scroll;
        gui::min_item_size({ widths[0], 65 });
        if (nr < (int) m_file_names.size()) {
            bool select = strcmp(m_file_names[nr].c_str(), m_file_name.data()) == 0;
            if (gui::button(m_file_names[nr].c_str(), select)) {
                strncpy(m_file_name.data(), m_file_names[nr].c_str(), m_file_name.size());
            }
        }
        else {
            gui::padding({});
        }
        gui::same_line();
        gui::separator();
        gui::padding({ widths[2], 0 });
    }
    gui::align(gui::CENTER);

    // scrollbar
    Vec c2 = gui::cursor();
    gui::cursor(c1);
    gui::min_item_size({ 65, c2.y - c1.y - gui::PADDING });
    gui::vertical_drag_int(m_file_scroll, 0, max_scroll, PAGE_LENGTH);
    gui::cursor(c2);
    gui::separator();


    // file buttons
    widths = calculate_column_widths({ -1, -1, -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Load")) {
        std::string name = m_file_name.data();
        if (name.empty()) {
            status("Load error: empty song name");
        }
        else if (std::find(m_file_names.begin(), m_file_names.end(), name) == m_file_names.end()) {
            status("Load error: song not listed");
        }
        else {
            std::string path = m_dir_name + name + FILE_SUFFIX;
            load_song(player::song(), path.c_str());
            status("Song was loaded");
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[1], 88 });
    if (gui::button("Save")) {
        std::string name = m_file_name.data();
        std::string path = m_dir_name + name + FILE_SUFFIX;
        if (!save_song(song, path.c_str())) {
            status("Save error: ?");
        }
        else {
            init_project_view();
            status("Song was saved");
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[2], 88 });
    if (gui::button("Delete")) {
        std::string name = m_file_name.data();
        if (name.empty()) {
            status("Delete error: empty song name");
        }
        else if (std::find(m_file_names.begin(), m_file_names.end(), name) == m_file_names.end()) {
            status("Delete error: song not listed");
        }
        else {
            std::string path = m_dir_name + name + FILE_SUFFIX;
            unlink(path.c_str());
            init_project_view();
            status("Song was deleted");
        }
    }

    // TODO: make it incremental
    gui::same_line();
    gui::min_item_size({ widths[3], 88 });
    if (gui::button("Export")) ogg_export();

    gui::separator();


    // status
    widths = calculate_column_widths({ -1 });
    gui::min_item_size({ widths[0], 88 });
    gui::align(gui::LEFT);
    gui::text(m_status_msg.c_str());
    gui::align(gui::CENTER);
    if (++m_status_age > 100) m_status_msg = "";
}

