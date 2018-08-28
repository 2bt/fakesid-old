#include "gui.hpp"
#include <vector>
#include <string>
#include <SDL.h>


namespace {


struct Span {
    enum Style {
        NORMAL,
        HIGHLIGHT,
        HEADLINE
    };
    Style       style;
    std::string text;
};

struct Line {
    std::vector<Span> spans;
    int               height;
};

enum {
    LINE_HEIGHT       = 60,
    HEADLINE_HEIGHT   = 75,
    PARAGRAPH_PADDING = 30,
};


std::vector<Line> m_lines;
int               m_text_height;
int               m_width;
int               m_scroll;




std::vector<char> read_help() {
    SDL_RWops* file = SDL_RWFromFile("help.md", "r");
    if (!file) {};
    std::vector<char> buffer(SDL_RWseek(file, 0, RW_SEEK_END) + 1);
    SDL_RWseek(file, 0, RW_SEEK_SET);
    SDL_RWread(file, buffer.data(), sizeof(uint8_t), buffer.size());
    SDL_RWclose(file);
    return buffer;
}


void init() {

    std::vector<char> buffer = read_help();


    m_width = calculate_column_widths({ -1, gui::SEPARATOR_WIDTH, 65 })[0];


    Span::Style style = Span::NORMAL;
    m_lines.emplace_back();
    m_lines.back().spans.push_back({ style });

    int width = 0;
    gfx::font(FONT_DEFAULT);
    for (char const* p = buffer.data(); char c = *p;) {
        ++p;
        if (c == '\n') {
            if (*p == '\n') {
                while (*p == '\n') {
                    ++p;
                    m_lines.emplace_back();
                }
                m_lines.emplace_back();
                m_lines.back().spans.push_back({ style });
                width = 0;
                continue;
            }
            if (m_lines.back().spans.back().text.empty()) continue;
            c = ' ';
        }

        if (c == '#') {
            if (m_lines.back().spans.size() == 1 && m_lines.back().spans.front().text.empty()) {
                while (*p == ' ') ++p;
                m_lines.back().spans.front().style = Span::HEADLINE;
                continue;
            }
        }


        if (c == '*') {
            if (*p != '*') {
                style = style == Span::NORMAL ? Span::HIGHLIGHT : Span::NORMAL;
                m_lines.back().spans.push_back({ style });
                continue;
            }
            ++p;
        }

        m_lines.back().spans.back().text += c;
        width += gfx::glyph_width(c);

        if (c == ' ') continue;

        if (width > m_width - 40) {
            width = 0;

            m_lines.emplace_back();
            Line& line      = m_lines[m_lines.size() - 2];
            Line& next_line = m_lines.back();
            next_line.spans.push_back({ style });
            Span& span      = line.spans.back();
            Span& next_span = next_line.spans.back();

            while (!span.text.empty() && span.text.back() != ' ') {
                width += gfx::glyph_width(span.text.back());
                next_span.text = span.text.back() + next_span.text;
                span.text.pop_back();
            }

        }
    }


    m_text_height = 0;
    for (Line& line : m_lines) {
        if (line.spans.empty() || (line.spans.size() == 1 && line.spans.front().text.empty())) {
            line.height = PARAGRAPH_PADDING;
        }
        else if (line.spans.front().style == Span::HEADLINE) {
            line.height = HEADLINE_HEIGHT;
        }
        else {
            line.height = LINE_HEIGHT;
        }
        m_text_height += line.height;
    }
}


} // namespace


void draw_help_view() {
    static bool init_done = false;
    if (!init_done) {
        init_done = true;
        init();
    }


    // prepare scrollbar
    Vec pos = gui::cursor() + Vec(gui::PADDING) + Vec(20, 0);
    int height = gfx::screensize().y - gui::SEPARATOR_WIDTH - gui::PADDING * 3 - 88 - pos.y;

    int max_scroll = std::max<int>(0, m_text_height) - height;
    if (m_scroll > max_scroll) m_scroll = max_scroll;


    gui::padding({ m_width, height });
    gui::same_line();
    gui::separator();
    gui::min_item_size({ 65, height });
    gui::vertical_drag_int(m_scroll, 0, max_scroll, height);

    gui::separator();


    gfx::font(FONT_DEFAULT);
    gui::align(gui::LEFT);


    Vec offset = { 0, -m_scroll + 10 };

    gfx::clip_rectangle(pos, { m_width, height });
    for (Line const& line : m_lines) {
        offset.x = 0;
        if (offset.y > height) break;
        if (offset.y + line.height > 0) {
            for (Span const& span : line.spans) {
                if (span.style == Span::NORMAL) gfx::color({ 255, 255, 255, 255 });
                else gfx::color({ 0x55, 0xa0, 0x49, 255 });


                gfx::print(pos + offset, span.text.c_str());

                int w = gfx::text_size(span.text.c_str()).x;
                if (span.style == Span::HEADLINE) {
                    gfx::rectangle(pos + offset + Vec(0, 60), { w, 8 }, 0);
                }
                offset.x += w;
            }
        }
        offset.y += line.height;
    }
    gfx::clear_clip_rectangle();

    gui::align(gui::CENTER);
}
