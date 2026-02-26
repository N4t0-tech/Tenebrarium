#include "XpLoader.hpp"
#include <ftxui/dom/elements.hpp>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <zlib.h>
#include <cstring>

using namespace ftxui;

static const char32_t kCp437[256] = {
    0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
};

static std::string cp437ToUtf8(char32_t cp) {
    std::string result;
    if (cp == 0) {
        result += ' ';  // null → espacio
    } else if (cp < 0x80) {
        result += static_cast<char>(cp);
    } else if (cp < 0x800) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return result;
}

static int32_t readInt32(const uint8_t* data, size_t& offset) {
    int32_t val;
    std::memcpy(&val, data + offset, 4);
    offset += 4;
    return val;
}

XpFile loadXp(const std::string& path) {
    // 1. Descomprimir directamente con gzopen (soporta gzip y zlib)
    gzFile gz = gzopen(path.c_str(), "rb");
    if (!gz) throw std::runtime_error("Cannot open .xp file: " + path);

    std::vector<uint8_t> data;
    uint8_t buf[4096];
    int n;
    while ((n = gzread(gz, buf, sizeof(buf))) > 0)
        data.insert(data.end(), buf, buf + n);
    gzclose(gz);

    // 3. Parsear binario
    size_t offset = 0;
    /*int32_t version =*/ readInt32(data.data(), offset);  // ignoramos version
    int32_t numLayers = readInt32(data.data(), offset);

    XpFile xp;
    for (int l = 0; l < numLayers; l++) {
        XpLayer layer;
        layer.width  = readInt32(data.data(), offset);
        layer.height = readInt32(data.data(), offset);

        layer.cells.resize(layer.width * layer.height);

        // Células en orden columna-mayor: columna 0 primero (todas sus filas), luego col 1, etc.
        for (int x = 0; x < layer.width; x++) {
            for (int y = 0; y < layer.height; y++) {
                XpCell cell;
                int32_t glyphVal = readInt32(data.data(), offset);
                uint8_t cp = static_cast<uint8_t>(glyphVal & 0xFF);
                cell.glyph = kCp437[cp];
                cell.fg_r = data[offset++];
                cell.fg_g = data[offset++];
                cell.fg_b = data[offset++];
                cell.bg_r = data[offset++];
                cell.bg_g = data[offset++];
                cell.bg_b = data[offset++];
                // Almacenar en orden fila-mayor para acceso fácil
                layer.cells[y * layer.width + x] = cell;
            }
        }

        xp.layers.push_back(std::move(layer));
    }

    return xp;
}

ftxui::Element xpToElement(const XpLayer& layer, int stepX, int stepY) {
    if (stepX < 1) stepX = 1;
    if (stepY < 1) stepY = 1;
    Elements rows;
    for (int y = 0; y < layer.height; y += stepY) {
        Elements cells;
        for (int x = 0; x < layer.width; x += stepX) {
            const XpCell& cell = layer.cells[y * layer.width + x];
            std::string glyph = cp437ToUtf8(cell.glyph);
            if (glyph.empty()) glyph = " ";

            auto elem = text(glyph)
                | color(Color::RGB(cell.fg_r, cell.fg_g, cell.fg_b))
                | bgcolor(Color::RGB(cell.bg_r, cell.bg_g, cell.bg_b));
            cells.push_back(elem);
        }
        rows.push_back(hbox(std::move(cells)));
    }
    return vbox(std::move(rows));
}

// Devuelve el color "efectivo" de una celda para el half-block:
// celdas de bloque lleno (█) o glifo nulo usan fg; espacios usan bg.
static Color effectiveColor(const XpCell& c) {
    // glyph U+2588 = █ (full block, CP437 219); glyph 0 = nulo → bg
    if (c.glyph == 0 || c.glyph == U' ')
        return Color::RGB(c.bg_r, c.bg_g, c.bg_b);
    // Para el resto confiamos en el fg como color dominante
    return Color::RGB(c.fg_r, c.fg_g, c.fg_b);
}

ftxui::Element xpToElementHalfBlock(const XpLayer& layer) {
    Elements rows;
    // Procesamos de 2 en 2 filas; si la altura es impar la última fila va sola
    for (int y = 0; y < layer.height; y += 2) {
        Elements cells;
        for (int x = 0; x < layer.width; x++) {
            const XpCell& top = layer.cells[y * layer.width + x];
            Color topCol = effectiveColor(top);
            Color botCol;
            if (y + 1 < layer.height) {
                const XpCell& bot = layer.cells[(y + 1) * layer.width + x];
                botCol = effectiveColor(bot);
            } else {
                botCol = Color::RGB(0, 0, 0);
            }
            // ▀ pinta la mitad superior con fg y la inferior con bg
            cells.push_back(text("▀") | color(topCol) | bgcolor(botCol));
        }
        rows.push_back(hbox(std::move(cells)));
    }
    return vbox(std::move(rows));
}
