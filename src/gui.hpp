#include "gfx.hpp"

namespace gui {
    enum { PADDING = 2 };

    void id(void* addr);
    void same_line();
    void min_item_size(Vec const& s);
    void begin_frame();
    void text(char const* fmt, ...);
    bool button(char const* label);
    bool drag_int(char const* label, int& value, int min, int max, int page = 1);

    void note(int& n, bool highlight);
    void clavier(int& n, int offset, bool highlight);
}
