#pragma once
#include "gfx.hpp"

namespace gui {
    enum { PADDING = 2 };
    Vec  cursor();
    void cursor(Vec c);
    void begin_frame();
    void id(void* addr);
    void same_line();
    void min_item_size(Vec const& s);
    void padding(Vec const& size);
    void text(char const* fmt, ...);
    void highlight();
    bool button(char const* label, bool active = false);
    bool hold();
    void block_touch();
    bool drag_int(char const* label, int& value, int min, int max, int page = 1);
    void clavier(int& n, int offset, bool highlight);
}
