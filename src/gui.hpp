#pragma once
#include "gfx.hpp"

enum { CLAVIER_WIDTH = 24 };

namespace gui {
    enum { PADDING = 2 };
    bool process_event(const SDL_Event& e);
    Vec  cursor();
    void begin_frame();
    void id(void const* addr);
    void same_line();
    void min_item_size(Vec const& s);
    void padding(Vec const& size);
    void separator();
    void text(char const* fmt, ...);
    void highlight();
    bool button(char const* label, bool active = false);
    bool hold();
    void block_touch();
    void input_text(char* str, int len);
    bool drag_int(char const* label, char const* fmt, int& value, int min, int max, int page = 1);
    bool clavier(uint8_t& n, int offset, bool highlight);
}
