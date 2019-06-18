#pragma once
#include "gfx.hpp"

enum { CLAVIER_WIDTH = 24 };

namespace gui {
    namespace color {
        constexpr SDL_Color make(uint32_t c, uint8_t a = 255) {
            return { uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), a};
        }
    }

    struct Box {
        Vec pos;
        Vec size;
        bool contains(Vec const& p) const {
            return p.x >= pos.x && p.y >= pos.y &&
                   p.x < pos.x + size.x && p.y < pos.y + size.y;
        }
    };

    enum {
        PADDING = 2,
        SEPARATOR_WIDTH = 6,
    };
    enum Align { CENTER, LEFT, RIGHT };

    bool process_event(const SDL_Event& e);
    Vec  cursor();
    void cursor(Vec const& c);
    void begin_frame();
    void id(void const* addr);
    void same_line();
    void next_line();
    void align(Align a);
    void min_item_size(Vec const& s);
    Box  padding(Vec const& size);
    void separator();
    void text(char const* fmt, ...);
    void highlight();
    bool button(char const* label, bool active = false);
    bool hold();
    void block_touch();
    void input_text(char* str, int len);
    bool drag_int(char const* label, char const* fmt, int& value, int min, int max, int page = 1);
    bool vertical_drag_int(int& value, int min, int max, int page = 1);
    bool clavier(uint8_t& n, int offset, bool highlight);

    template<class T>
    bool drag_int(char const* label, char const* fmt, T& value, int min, int max, int page = 1) {
        int v = value;
        id(&value);
        bool b = drag_int(label, fmt, v, min, max, page);
        if (b) value = v;
        return b;
    }
}


#include <vector>
inline std::vector<int> calculate_column_widths(std::vector<int> weights, int absolute = 0) {
    absolute = absolute ?: gfx::screensize().x - gui::PADDING;
    int relative = 0;
    for (int w : weights) {
        if (w > 0) {
            absolute -= w + gui::PADDING;
        }
        else {
            absolute -= gui::PADDING;
            relative += -w;
        }
    }
    std::vector<int> widths;
    widths.reserve(weights.size());
    for (int w : weights) {
        if (w > 0) {
            widths.emplace_back(w);
        }
        else {
            int q = absolute * -w / relative;
            absolute -= q;
            relative += w;
            widths.emplace_back(q);
        }
    }
    return widths;
}
