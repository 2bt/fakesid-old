#pragma once

namespace wavelog {
    bool open(char const* name);
    void write(short const* buffer, int len);
    void close();
}
