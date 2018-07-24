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
    const SDL_Color button_normal = make(0x555555, 255);
    const SDL_Color button_hover  = make(0x995555, 255);
    const SDL_Color button_active = make(0xaa5555, 255);

    const SDL_Color drag          = make(0x222222, 255);
    const SDL_Color handle_normal = make(0x885555, 255);
    const SDL_Color handle_active = make(0xaa5555, 255);

    const SDL_Color separator     = make(0x222222, 255);

    const SDL_Color highlight     = make(0xbbbbbb, 255);

    const SDL_Color note          = make(0xcc5555, 255);
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
bool        m_highlight;
bool        m_same_line;
void const* m_id;
void const* m_active_item;
std::array<char, 1024> m_text_buffer;


void const* get_id(void const* addr) {
    if (m_id) {
        addr = m_id;
        m_id = nullptr;
   }
   return addr;
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


void cursor(Vec c) { m_cursor_max = c; }


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


void padding(Vec const& size) {
    item_box(size);
}

void separator() {
    enum { WIDTH = 6 };
    gfx::color(color::separator);
	if (m_same_line) {
		Box box = item_box({ WIDTH, m_cursor_max.y - m_cursor_min.y - PADDING });
		m_same_line = true;
        gfx::rectangle(box.pos + Vec(0, -PADDING), box.size + Vec(0, PADDING * 2), 0);
    }
	else {
		Box box = item_box({ m_cursor_max.x - m_cursor_min.x - PADDING, WIDTH });
        gfx::rectangle(box.pos + Vec(-PADDING, 0), box.size + Vec(PADDING * 2, 0), 0);
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


void highlight() { m_highlight = true; }


bool button(char const* label, bool active) {
    m_hold = false;
    Vec s = gfx::text_size(label);
    Box box = item_box(s + Vec(30, 10));
    SDL_Color color = color::button_normal;
    bool clicked = false;
    if (m_active_item == nullptr && box.touched()) {
        color = color::button_hover;
        if (box.contains(m_prev_touch_pos)) {
            if (++m_hold_count > 30) m_hold = true;
        }
        else m_hold_count = 0;
        if (input::just_released()) clicked = true;
    }
    else {
        if (active) color = color::button_active;
        else if (m_highlight) color = color::mix(color, color::highlight, 0.25);
    }
    m_highlight = false;
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
    void const* id = get_id(label);
    print_to_text_buffer(label, value);
    Vec s = gfx::text_size(m_text_buffer.data());

    Box box = item_box(s + Vec(30, 10));
	int handle_w = box.size.x * page / (max - min + page);
	int handle_x = (value - min) * (box.size.x - handle_w)  / (max - min);

    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        m_active_item = id;
    }
    int old_value = value;
    if (m_active_item == id) {
        int x = m_touch_pos.x - box.pos.x;
		int v = min + (x - handle_w * (page - 1) / (2 * page)) * (max - min) / (box.size.x - handle_w);
		value = glm::clamp(v, min, max);
    }

    gfx::color(color::drag);
    gfx::rectangle(box.pos, box.size, 0);

    gfx::color(m_active_item == id ? color::handle_active : color::handle_normal);
    gfx::rectangle(box.pos + Vec(handle_x, 0), { handle_w, box.size.y }, 0);

    gfx::color(color::make(0xffffff));
    gfx::print(box.pos + box.size / 2 - s / 2, m_text_buffer.data());

    return value != old_value;
}


void clavier(uint8_t& n, int offset, bool highlight) {
    void const* id = get_id(&n);
    int w = gfx::screensize().x - PADDING - m_cursor_max.x;
    Box box = item_box({ w, 65 });

    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        m_active_item = id;
    }

    enum { COLS = 21 };

    int x0 = 0;
    for (int i = 0; i < COLS; ++i) {
        int x1 = w * (i + 1) / COLS;
        int nn = i + 1 + offset;
        Box b = {
            { box.pos.x + x0, box.pos.y },
            { x1 - x0, box.size.y },
        };
        bool touch = b.contains({ m_touch_pos.x, b.pos.y });
        b.size.x -= PADDING;
        if (m_active_item == id && touch) {
            if (input::just_pressed()) {
                if (n == nn) n = 0;
                else if (n == 0) n = nn;
            }
            else if (n != 0) n = nn;
        }
        SDL_Color color = color::make(0x333333);
        if ((i + offset) % 12 == 0) color = color::make(0x444444);
        if ((1 << (i + offset) % 12) & 0b010101001010) color = color::make(0x222222);
        if (m_active_item == id) color = color::mix(color, color::handle_active, 0.2);
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
