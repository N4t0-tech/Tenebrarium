#include "TerminalScreen.hpp"
#include <raylib.h>

TerminalScreen::TerminalScreen(int cols, int rows, int cellW, int cellH, Font font)
    : cols_(cols), rows_(rows), cellW_(cellW), cellH_(cellH),
      font_(font), buf_(cols * rows)
{}

void TerminalScreen::clear() {
    for (auto& c : buf_) {
        c.codepoint = ' ';
        c.fg        = WHITE;
        c.bg        = BLACK;
        c.flags     = 0;
    }
}

void TerminalScreen::put(int col, int row, int codepoint,
                         Color fg, Color bg, uint8_t flags) {
    if (!inBounds(col, row)) return;
    Cell& c   = buf_[idx(col, row)];
    c.codepoint = codepoint;
    c.fg        = fg;
    c.bg        = bg;
    c.flags     = flags;
}

// Avanza por los codepoints UTF-8 de la string
int TerminalScreen::putStr(int col, int row, const std::string& utf8,
                           Color fg, Color bg, uint8_t flags) {
    int startCol = col;
    const unsigned char* s = reinterpret_cast<const unsigned char*>(utf8.c_str());
    while (*s) {
        int cp;
        int len;
        if      (*s < 0x80)  { cp = *s;                                    len = 1; }
        else if (*s < 0xE0)  { cp = (*s & 0x1F) << 6  | (s[1] & 0x3F);   len = 2; }
        else if (*s < 0xF0)  { cp = (*s & 0x0F) << 12 | (s[1] & 0x3F) << 6
                                                        | (s[2] & 0x3F);   len = 3; }
        else                 { cp = (*s & 0x07) << 18 | (s[1] & 0x3F) << 12
                                   | (s[2] & 0x3F) << 6 | (s[3] & 0x3F); len = 4; }
        put(col++, row, cp, fg, bg, flags);
        s += len;
    }
    return col - startCol;
}

Color TerminalScreen::dimColor(Color c) {
    return { (uint8_t)(c.r / 2), (uint8_t)(c.g / 2),
             (uint8_t)(c.b / 2), c.a };
}

void TerminalScreen::render() const {
    float fs = static_cast<float>(cellH_);

    for (int row = 0; row < rows_; row++) {
        for (int col = 0; col < cols_; col++) {
            const Cell& c = buf_[idx(col, row)];
            int px = col * cellW_;
            int py = row * cellH_;

            Color fg = c.fg;
            Color bg = c.bg;
            if (c.flags & CELL_INVERTED) std::swap(fg, bg);
            if (c.flags & CELL_DIM)      fg = dimColor(fg);

            // Fondo de celda
            DrawRectangle(px, py, cellW_, cellH_, bg);

            // Glifo
            if (c.codepoint > 32) {
                DrawTextCodepoint(font_, c.codepoint,
                                  { (float)px, (float)py }, fs, fg);
            }
        }
    }
}
