#include "wavelog.hpp"
#include "player.hpp"
#include <SDL.h>
#include <sndfile.h>


namespace wavelog {
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


} // namespace


bool open(char const* name) {

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


void write(short const* buffer, int len) {
    if (m_sndfile) sf_writef_short(m_sndfile, buffer, len);
}


void close() {
    sf_close(m_sndfile);
    SDL_RWclose(m_file);
    m_sndfile = nullptr;
    m_file = nullptr;
}


} // namespace


