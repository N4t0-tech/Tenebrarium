#pragma once

#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <cstdint>

struct XpCell {
    char32_t glyph;        // Unicode codepoint (convertido desde CP437)
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;
};

struct XpLayer {
    int width, height;
    std::vector<XpCell> cells;  // row-major after loading (cell at x,y = cells[y*width+x])
};

struct XpFile {
    std::vector<XpLayer> layers;
};

// Carga un archivo .xp de REXPaint (descomprime zlib y parsea binario)
XpFile loadXp(const std::string& path);

// Convierte una capa XP a un Element FTXUI con colores truecolor.
// stepX/stepY: 1=tamaño original, 2=mitad, etc.
ftxui::Element xpToElement(const XpLayer& layer, int stepX = 1, int stepY = 2);

// Versión half-block: empaqueta 2 filas del .xp en 1 fila de terminal usando ▀.
// Sin pérdida de píxeles verticales y aspecto cuadrado automático.
ftxui::Element xpToElementHalfBlock(const XpLayer& layer);
