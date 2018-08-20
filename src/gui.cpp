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


    const SDL_Color button_normal = make(0x505050);
    const SDL_Color button_active = make(0x55a049);
    const SDL_Color button_hover = make(0x94e089);

    const SDL_Color input_text_normal = make(0x222222);
    const SDL_Color input_text_hover  = button_hover;
    const SDL_Color input_text_active = button_active;

    const SDL_Color drag          = make(0x222222);
    const SDL_Color handle_normal = button_active;
    const SDL_Color handle_active = button_hover;

    const SDL_Color separator     = make(0x111111);

    const SDL_Color highlight     = make(0x787878);

    const SDL_Color note_normal   = handle_normal;
    const SDL_Color note_active   = handle_active;

    const SDL_Color text          = make(0xffffff);
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
char*       m_input_text_str = nullptr;
int         m_input_text_len;
int         m_input_text_pos;
int         m_input_cursor_blink = 0;

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


Vec cursor() {
    if (m_same_line) return Vec(m_cursor_max.x, m_cursor_min.y);
    else return Vec(m_cursor_min.x, m_cursor_max.y);
}

void cursor(Vec const& c) {
    if (m_same_line) {
        m_cursor_max.x = c.x;
        m_cursor_min.y = c.y;
    }
    else {
        m_cursor_min.x = c.x;
        m_cursor_max.y = c.y;
    }
}


void id(void const* addr) {
    if (!m_id) m_id = addr;
}


void same_line() {
    m_same_line = true;
}


void next_line() {
    m_same_line = false;
}


void min_item_size(Vec const& s) {
    m_min_item_size = s;
}


void begin_frame() {
    ++m_input_cursor_blink;
    m_prev_touch_pos = m_touch_pos;
    m_touch_pos = input::touch(0).pos;
    m_cursor_min = { 0, 0 };
    m_cursor_max = { 0, 0 };
    m_same_line = false;
    if (input::released()) {
        m_active_item = nullptr;
        m_hold_count = 0;
    }
    if (input::just_pressed() && m_input_text_str) {
        m_input_text_str = nullptr;
        SDL_StopTextInput();
    }
}


void padding(Vec const& size) {
    item_box(size);
}

void separator() {
    gfx::color(color::separator);
    if (m_same_line) {
        Box box = item_box({ SEPARATOR_WIDTH, m_cursor_max.y - m_cursor_min.y - PADDING });
        m_same_line = true;
        gfx::rectangle(box.pos + Vec(0, -PADDING), box.size + Vec(0, PADDING * 2), 0);
    }
    else {
        Box box = item_box({ m_cursor_max.x - m_cursor_min.x - PADDING, SEPARATOR_WIDTH });
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
    gfx::color(color::text);
    gfx::print(box.pos + box.size / 2 - s / 2, m_text_buffer.data());
}


void highlight() { m_highlight = true; }


bool button(char const* label, bool active) {
    enum { HOLD_TIME = 10 };
    m_hold = false;
    Vec s = gfx::text_size(label);
    Box box = item_box(s + Vec(30, 10));
    SDL_Color color = color::button_normal;
    bool clicked = false;
    if (m_active_item == nullptr && box.touched()) {
        color = color::button_hover;
        if (box.contains(m_prev_touch_pos)) {
            if (++m_hold_count > HOLD_TIME) m_hold = true;
        }
        else m_hold_count = 0;
        if (input::just_released()) clicked = true;
    }
    else {
        if (active) color = color::button_active;
        else if (m_highlight) color = color::highlight;
    }
    m_highlight = false;
    gfx::color(color);
    gfx::rectangle(box.pos, box.size, 2);
    gfx::color(color::text);
    gfx::print(box.pos + box.size / 2 - s / 2, label);
    return clicked;
}


bool hold() { return m_hold; }


void block_touch() {
    m_active_item = (void const*) -1;
}


bool process_event(const SDL_Event& e) {
    char c;
    switch (e.type) {
    case SDL_KEYDOWN:
        if (!m_input_text_str) return false;
        switch (e.key.keysym.scancode) {
        case SDL_SCANCODE_RETURN:
            SDL_StopTextInput();
            m_input_text_str = nullptr;
            break;
        case SDL_SCANCODE_BACKSPACE:
            m_input_cursor_blink = 0;
            if (m_input_text_pos > 0) {
                m_input_text_str[--m_input_text_pos] = '\0';
            }
            break;
        default:
            break;
        }
        return true;
    case SDL_TEXTINPUT:
        if (!m_input_text_str) return false;
        m_input_cursor_blink = 0;
        c = e.text.text[0];
        if ((isalnum(c) || strchr("_-.", c)) && m_input_text_pos < m_input_text_len) {
            m_input_text_str[m_input_text_pos++] = c;
        }
        return true;
    default:
        return false;
    }
}


void input_text(char* str, int len) {
    Vec s = gfx::text_size(str);
    Box box = item_box(s + Vec(30, 10));

    SDL_Color color = color::input_text_normal;
    if (m_active_item == nullptr && box.touched()) {
        color = color::input_text_hover;
        if (input::just_released()) {
            // start keyboard
            SDL_StartTextInput();
            m_input_text_str = str;
            m_input_text_len = len;
            m_input_text_pos = strlen(m_input_text_str);
        }
    }
    if (m_input_text_str == str) {
        color = color::input_text_active;
    }

    gfx::color(color::input_text_normal);
    gfx::rectangle(box.pos, box.size, 0);
    gfx::color(color);
    gfx::rectangle(box.pos, box.size, 4);
    gfx::color(color::text);
    gfx::print(box.pos + box.size / 2 - s / 2, str);
    // cursor
    if (m_input_text_str == str && m_input_cursor_blink % 16 < 8) {
        gfx::print(box.pos + box.size / 2 + Vec(s.x, -s.y) / 2, "_");
    }
}


bool drag_int(char const* label, char const* fmt, int& value, int min, int max, int page) {
    gfx::font(FONT_DEFAULT);
    Vec s1 = gfx::text_size(label);
    gfx::font(FONT_MONO);
    print_to_text_buffer(fmt, value);
    Vec s2 = gfx::text_size(m_text_buffer.data());
    // padding
    if (s2.x > 0) s1.x += 30;

    Box box = item_box(Vec(s1.x + s2.x, std::max(s1.y, s2.y)) + Vec(30, 10));
    int range = max - min;
    int handle_w = box.size.x * page / (range + page);
    int handle_x = range == 0 ? 0 : (value - min) * (box.size.x - handle_w) / range;

    void const* id = get_id(&value);
    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        m_active_item = id;
    }
    int old_value = value;
    if (m_active_item == id) {
        int x = m_touch_pos.x - box.pos.x;
        int v = min;
        if (range > 0) v += (x - handle_w * (page - 1) / (2 * page)) * range / (box.size.x - handle_w);
        value = glm::clamp(v, min, max);
    }

    gfx::color(color::drag);
    gfx::rectangle(box.pos, box.size, 0);

    gfx::color(m_active_item == id ? color::handle_active : color::handle_normal);
    gfx::rectangle(box.pos + Vec(handle_x, 0), { handle_w, box.size.y }, 0);


    gfx::font(FONT_DEFAULT);
    gfx::color(color::text);
    gfx::print(box.pos + Vec(15, box.size.y / 2 - s1.y / 2), label);

    gfx::font(FONT_MONO);
    gfx::color(color::text);
    gfx::print(box.pos + Vec(box.size.x - s2.x - 15, box.size.y / 2 - s2.y / 2), m_text_buffer.data());

    return value != old_value;
}

bool vertical_drag_int(int& value, int min, int max, int page) {
    Box box = item_box({});
    int range = max - min;
    int handle_h = box.size.y * page / (range + page);
    int handle_y = range == 0 ? 0 : (value - min) * (box.size.y - handle_h) / range;

    void const* id = get_id(&value);
    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        m_active_item = id;
    }
    int old_value = value;
    if (m_active_item == id) {
        int y = m_touch_pos.y - box.pos.y;
        int v = min;
        if (range > 0) v += (y - handle_h * (page - 1) / (2 * page)) * range / (box.size.y - handle_h);
        value = glm::clamp(v, min, max);
    }

    gfx::color(color::drag);
    gfx::rectangle(box.pos, box.size, 0);

    gfx::color(m_active_item == id ? color::handle_active : color::handle_normal);
    gfx::rectangle(box.pos + Vec(0, handle_y), { box.size.x, handle_h }, 0);

    return value != old_value;
}



bool clavier(uint8_t& n, int offset, bool highlight) {
    Box box = item_box({ 100, 65 });

    void const* id = get_id(&n);
    if (m_active_item == nullptr && box.touched() && input::just_pressed()) {
        m_active_item = id;
    }

    uint8_t old_n = n;

    int x0 = 0;
    bool just_pressed = input::just_pressed();
    for (int i = 0; i < CLAVIER_WIDTH; ++i) {
        int x1 = (box.size.x - (CLAVIER_WIDTH - 1) * PADDING) * (i + 1) / CLAVIER_WIDTH + (i + 1) * PADDING;
        int nn = i + 1 + offset;
        Box b = {
            { box.pos.x + x0, box.pos.y },
            { x1 - x0, box.size.y },
        };
        bool touch = b.contains({ m_touch_pos.x, b.pos.y });
        b.size.x -= PADDING;
        if (m_active_item == id && touch) {
            if (just_pressed) {
                if (n == nn) n = 0;
                else if (n == 0) n = nn;
            }
            else if (n != 0) n = nn;
        }
        SDL_Color color = color::make(0x222222);
        if ((i + offset) % 12 == 0) color = color::make(0x333333);
        if ((1 << (i + offset) % 12) & 0b010101001010) color = color::make(0x111111);
        if (highlight) color = color::mix(color, color::highlight, 0.2);
        gfx::color(color);
        gfx::rectangle( b.pos, b.size, 0);

        if (n == nn) {
            if (m_active_item == id && touch) gfx::color(color::note_active);
            else gfx::color(color::note_normal);
            gfx::rectangle( b.pos, b.size, 2);
        }

        x0 = x1;
    }
    return n != old_n;
}


} // namespace
