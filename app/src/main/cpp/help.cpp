#include "gui.hpp"
#include <vector>
#include <string>


namespace {


char const* m_raw_text = R"(
*1. Introduction*

Fake SID is a chiptune tracker that let's you create Commodore 64 music.
Unlike other C64 trackers, songs made with Fake SID cannot be played on real C64 directly,
but this is mainly an export issue.

The original SID (the sound chip of the C64) has three voices, i.e. you can play three sounds simultaniously.
In Fake SID you have a fourth voice at your disposal, although true purists abstain from using it.

A *song* in Fake SID is basically a table with one column per voice.
Table entries are references to so-called tracks.

*Tracks* represent one bar of music for one voice.
They contain notes and references to instruments and effects.

*Instruments* control the volume and waveform of a voice.
They may also control the filter.

*Effects* - together with notes - control the pitch of a voice.

At the top of the screen you find a tab for each of these categories.
Let's go through each and discuss them in more detail.


*2. Project*

Here you set the title, author, track length, and tempo of the current song.
Additionally, songs can be loaded, saved, and exported.


*3. Song*

The song tab shows you the 
The song table contains references to tracks.


*4. Track*


*5. Instrument*

Instruments pomprise volume control via *envelope*, the *wave* table and the optional *filter* table.



*6. Effect*

Effects control the pitch in correspondence with notes.
An effect is a table of pitch offsets.



End.)";


struct Span {
    enum Style { NORMAL, HIGHLIGHT };
    Style       style;
    std::string text;
};

struct Line {
    std::vector<Span> spans;
    int               height;
};

enum {
    LINE_HEIGHT       = 60,
    PARAGRAPH_PADDING = 30,
};


std::vector<Line> m_lines;
int               m_text_height;
int               m_width;


void init() {



    m_width = calculate_column_widths({ -1, gui::SEPARATOR_WIDTH, 65 })[0];

    Span::Style style = Span::NORMAL;

    m_lines.emplace_back();
    m_lines.back().spans.push_back({ style });

    int width = 0;
    gfx::font(FONT_DEFAULT);
    for (char const* p = m_raw_text; char c = *p++;) {
        if (c == '\n') {
            if (*p == '\n') {
                m_lines.emplace_back();
                m_lines.emplace_back();
                m_lines.back().spans.push_back({ style });
                width = 0;
                continue;
            }
            if (m_lines.back().spans.back().text.empty()) continue;
            c = ' ';
        }


        if (c == '*') {
            if (*p != '*') {
                style = style == Span::NORMAL ? Span::HIGHLIGHT : Span::NORMAL;
                m_lines.back().spans.push_back({ style });
                continue;
            }
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
        else {
            line.height = LINE_HEIGHT;
        }
        m_text_height += line.height;
    }
}


int m_help_scroll;


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
    if (m_help_scroll > max_scroll) m_help_scroll = max_scroll;


    gui::padding({ m_width, height });
    gui::same_line();
    gui::separator();
    gui::min_item_size({ 65, height });
    gui::vertical_drag_int(m_help_scroll, 0, max_scroll, height);

    gui::separator();


    gfx::font(FONT_DEFAULT);
    gui::align(gui::LEFT);


    Vec offset = { 0, -m_help_scroll + 10 };

    gfx::clip_rectangle(pos, { m_width, height });
    for (Line const& line : m_lines) {
        offset.x = 0;
        if (offset.y > height) break;
        if (offset.y + line.height > 0) {
            for (Span const& span : line.spans) {
                if (span.style == Span::NORMAL) gfx::color({ 255, 255, 255, 255 });
                else gfx::color({ 0x55, 0xa0, 0x49, 255 });
                gfx::print(pos + offset, span.text.c_str());
                offset.x += gfx::text_size(span.text.c_str()).x;
            }
        }
        offset.y += line.height;
    }
    gfx::clear_clip_rectangle();

    gui::align(gui::CENTER);
}
