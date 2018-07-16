#include "gfx.hpp"

namespace gui {
    enum { PADDING = 2 };
    Vec  cursor();
    void begin_frame();
    void id(void* addr);
    void same_line();
    void min_item_size(Vec const& s);
    void text(char const* fmt, ...);
    bool button(char const* label, bool highlight = false);
    bool drag_int(char const* label, int& value, int min, int max, int page = 1);
    void note(int& n, bool highlight);
    void clavier(int& n, int offset, bool highlight);
}
