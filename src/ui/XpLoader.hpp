#pragma once

#include "ui/TerminalScreen.hpp"
#include <string>
#include <vector>
#include <cstdint>

struct XpCell {
    char32_t glyph;
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
};

struct XpLayer {
    int width, height;
    std::vector<XpCell> cells;  // row-major: cell(x,y) = cells[y*width+x]
};

struct XpFile {
    std::vector<XpLayer> layers;
};

// Carga un archivo .xp REXPaint (gzip + binario)
XpFile loadXp(const std::string& path);

// Vuelca una capa XP en el TerminalScreen en (col, row) con half-block (▀)
void xpDrawHalfBlock(TerminalScreen& scr, const XpLayer& layer, int col, int row, float scale = 1.0f);

// Vuelca una capa XP usando los glyphs CP437 reales (conserva texturas/detalles)
void xpDrawGlyphs(TerminalScreen& scr, const XpLayer& layer, int col, int row, float scale = 1.0f);
