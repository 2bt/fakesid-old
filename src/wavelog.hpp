#pragma once

namespace wavelog {
    bool init(int mixrate);
    void write(short const* buffer, int len);
    bool free();
}
