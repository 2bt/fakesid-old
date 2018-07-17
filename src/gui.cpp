#include "gui.hpp"
#include "input.hpp"


namespace gui {
namespace {


namespace color {
    constexpr SDL_Color make(uint32_t c, uint8_t a = 255) {
        return { uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), a};
    }
    SDL_Color mix(SDL_Color a, SDL_Color b, float x) {
        float w = (1 - x);
        return {
            uint8_t(a.r * w + b.r * x),
            uint8_t(a.g * w + b.g * x),
            uint8_t(a.b * w + b.b * x),
            uint8_t(a.a * w + b.a * x),
        };
    }
    const SDL_Color normal = make(0x666666, 200);
    const SDL_Color active = make(0x996666, 200);

    const SDL_Color drag_normal = make(0x666666, 80);
    const SDL_Color drag_active = make(0x996666, 80);
    const SDL_Color drag_handle = make(0xcc6666, 200);

    const SDL_Color highlight = make(0xffff00, 200);

    const SDL_Color note      = make(0xcc6666, 200);
}


struct Box {
    Vec pos;
    Vec size;
    bool contains(Vec const& p) const {
        return p.x >= pos.x && p.y >= pos.y &&
               p.x < pos.x + size.x && p.y < pos.y + size.y;
    }

    bool touched() const {
        return !input::released() && contains(input::touch(0).pos);
    }
};


Vec         m_cursor_min;
Vec         m_cursor_max;
Vec         m_min_item_size = {0, 0};
Vec         m_touch_pos;
Vec         m_prev_touch_pos;
int         m_hold_count;
bool        m_hold;
bool        m_same_line;
void const* m_id;
void const* m_current_item;
void const* m_active_item;
std::array<char, 1024> m_text_buffer;



void set_current_item(void const* addr) {
    if (m_id) {
        m_current_item = m_id;
        m_id = nullptr;
    }
    else {
        m_current_item = addr;
    }
}


void set_active_item() {
    m_active_item = m_current_item;
}


bool is_active_item() {
    return m_current_item == m_active_item;
}


Box item_box(Vec const& size) {
    Box box;
    box.size = glm::max(size, m_min_item_size);
    m_min_item_size = { 0, 0 };

    if (m_same_line) {
        m_same_line = false;
        box.pos = Vec(m_cursor_max.x, m_cursor_min.y) + Vec(PADDING);
        if (m_cursor_max.y - m_cursor_min.y - PADDING > box.size.y) {
            box.pos.y += (m_cursor_max.y - m_cursor_min.y - PADDING - box.size.y) / 2;
        }
        m_cursor_max = glm::max(m_cursor_max, box.pos + box.size);
    }
    else {
        box.pos = Vec(m_cursor_min.x, m_cursor_max.y) + Vec(PADDING);
        m_cursor_min.y = m_cursor_max.y;
        m_cursor_max = box.pos + box.size;
    }
    return box;
}


void print_to_text_buffer(const char* fmt, va_list args) {
    vsnprintf(m_text_buffer.data(), m_text_buffer.size(), fmt, args);
}


void print_to_text_buffer(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_to_text_buffer(fmt, args);
    va_end(args);
}


} // namespace


Vec cursor() { return m_cursor_max; }


void id(void* addr) {
    m_id = addr;
}


void same_line() {
    m_same_line = true;
}


void min_item_size(Vec const& s) {
    m_min_item_size = s;
}


void begin_frame() {
    m_prev_touch_pos = m_touch_pos;
    m_touch_pos = input::touch(0).pos;
    m_cursor_min = { 0, 0 };
    m_cursor_max = { 0, 0 };
    m_same_line = false;
    if (input::released()) {
        m_active_item = nullptr;
        m_hold_count = 0;
    }
}


void text(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_to_text_buffer(fmt, args);
    va_end(args);
    Vec s = gfx::text_size(m_text_buffer.data());
    Box box = item_box(s);
    gfx::color(color::make(0xffffff));
    gfx::print(box.pos + box.size / 2 - s / 2, m_text_buffer.data());
}


bool button(char const* label, bool highlight) {
    m_hold = false;
    Vec s = gfx::text_size(label);
    Box box = item_box(s + Vec(30, 10));
    SDL_Color color;
    bool clicked = false;
    if (m_active_item == nullptr && box.touched()) {
        color = color::active;
        if (box.contains(m_prev_touch_pos)) {
            if (++m_hold_count > 30) m_hold = true;
        }
        else m_hold_count = 0;
        if (input::just_released()) clicked = true;
    }
    else {
        color = color::normal;
        if (highlight) color = color::mix(color, color::highlight, 0.25);
    }
    gfx::color(color);
    gfx::rectangle(box.pos, box.size, 2);
    gfx::color(color::make(0xffffff));
    gfx::print(box.pos + box.size / 2 - s / 2, label);
    return clicked;
}


bool hold() { return m_hold; }


void block_touch() {
    m_active_item = (void const*) -1;
}


bool drag_int(char const* label, int& value, int min, int max, int page) {
    set_current_item(label);
    print_to_text_buffer(label, value);
    Vec s = gfx::text_size(m_text_buffer.data());

    Box box = item_box(s + Vec(30, 10));
	int handle_w = box.size.x * page / (max - min + page);
	int handle_x = (value - min) * (box.size.x - handle_w)  / (max - min);

    SDL_Color color = color::drag_normal;
    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        set_active_item();
    }
    int old_value = value;
    if (is_active_item()) {
        color = color::drag_active;
        int x = m_touch_pos.x - box.pos.x;
		int v = min + (x - handle_w * (page - 1) / (2 * page)) * (max - min) / (box.size.x - handle_w);
		value = glm::clamp(v, min, max);
    }

    gfx::color(color);
    gfx::rectangle(box.pos, box.size, 0);

    gfx::color(color::drag_handle);
    gfx::rectangle(box.pos + Vec(handle_x, 0), { handle_w, box.size.y }, 0);

    gfx::color(color::make(0xffffff));
    gfx::print(box.pos + box.size / 2 - s / 2, m_text_buffer.data());

    return value != old_value;
}


void clavier(int& n, int offset, bool highlight) {
    set_current_item(&n);
    int w = gfx::screensize().x - PADDING - m_cursor_max.x;
    Box box = item_box({ w, 65 });

    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        set_active_item();
    }

    enum { COLS = 21 };

    int x0 = 0;
    for (int i = 0; i < COLS; ++i) {
        int x1 = w * (i + 1) / COLS;
        int nn = i + 1 + offset;
        Box b = {
            { box.pos.x + x0, box.pos.y },
            { x1 - x0 - PADDING, box.size.y },
        };
        bool touch = b.contains({ m_touch_pos.x, b.pos.y });
        if (is_active_item() && touch) {
            if (input::just_pressed()) {
                if (n == nn) n = 0;
                else if (n == 0) n = nn;
            }
            else if (n != 0) n = nn;
        }
        SDL_Color color = color::make(0x333333);
        if ((i + offset) % 12 == 0) color = color::make(0x444444);
        if ((1 << (i + offset) % 12) & 0b010101001010) color = color::make(0x222222);
        if (is_active_item()) color = color::mix(color, color::active, 0.2);
        else if (highlight) color = color::mix(color, color::highlight, 0.1);
        gfx::color(color);
        gfx::rectangle( b.pos, b.size, 0);

        if (n == nn) {
            gfx::color(color::note);
            gfx::rectangle( b.pos, b.size, 2);
        }

        x0 = x1;
    }
}


} // namespace
