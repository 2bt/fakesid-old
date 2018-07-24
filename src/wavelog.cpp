#include <sndfile.h>
#include "wavelog.hpp"


namespace wavelog {

#ifdef __ANDROID__

bool init(int mixrate) { return true; }
void write(short const* buffer, int len) {}
bool free() {}

#else

namespace {

SNDFILE* m_log;

} // namespace


bool init(int mixrate) {
    SF_INFO info = { 0, mixrate, 1, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
    m_log = sf_open("log.wav", SFM_WRITE, &info);
    return m_log != nullptr;
}


void write(short const* buffer, int len) {
	sf_writef_short(m_log, buffer, len);
}


bool free() {
    sf_close(m_log);
}

#endif

} // namespace
