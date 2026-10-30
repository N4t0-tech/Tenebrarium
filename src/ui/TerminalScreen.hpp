#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <cstdint>

// Flags de celda
static constexpr uint8_t CELL_BOLD         = 1;
static constexpr uint8_t CELL_DIM          = 2;
static constexpr uint8_t CELL_INVERTED     = 4;
static constexpr uint8_t CELL_STRIKETHROUGH = 8;
static constexpr uint8_t CELL_ITALIC       = 16;

struct Cell {
    int      codepoint = ' ';
    Color    fg        = WHITE;
    Color    bg        = BLACK;
    uint8_t  flags     = 0;
};

// Glifo del atlas CP437 en una celda cuadrada, en píxeles relativos al grid
struct GlyphTile {
    int     x, y, size;
    uint8_t cp;
    Color   fg, bg;
};

// Buffer de celdas que el Renderer escribe y Raylib dibuja.
class TerminalScreen {
public:
    // fontH: tamaño real de render de la fuente (0 = usar cellH)
    TerminalScreen(int cols, int rows, int cellW, int cellH, Font font, Font fontItalic = {}, int fontH = 0);

    int cols()  const { return cols_; }
    int rows()  const { return rows_; }
    int cellW() const { return cellW_; }
    int cellH() const { return cellH_; }

    void clear();
    void put(int col, int row, int codepoint,
             Color fg, Color bg = BLACK, uint8_t flags = 0);
    // Escribe string UTF-8; devuelve columnas consumidas
    int  putStr(int col, int row, const std::string& utf8,
                Color fg, Color bg = BLACK, uint8_t flags = 0);
    // Encola un glifo del atlas CP437; se dibuja encima de las celdas
    void putTile(int px, int py, int size, uint8_t cp, Color fg, Color bg);
    // Atlas CP437 de 16×16 glifos, blancos con alfa
    static void setTileAtlas(Texture2D atlas) { tileAtlas_ = atlas; }
    static bool hasTileAtlas() { return tileAtlas_.id != 0; }
    // Dibuja los glifos de las celdas con el atlas CP437 en vez de la fuente (celdas cuadradas)
    void setAtlasCells(bool on) { atlasCells_ = on; }
    // Dibuja el buffer (llamar entre BeginDrawing/EndDrawing)
    void render(int offsetX = 0, int offsetY = 0) const;

    static Color dimColor(Color c);

private:
    int   cols_, rows_, cellW_, cellH_, fontH_;
    Font  font_;
    Font  fontItalic_;
    std::vector<Cell> buf_;
    std::vector<GlyphTile> tiles_;
    bool atlasCells_ = false;
    static inline Texture2D tileAtlas_{};

    bool inBounds(int col, int row) const {
        return col >= 0 && col < cols_ && row >= 0 && row < rows_;
    }
    int idx(int col, int row) const { return row * cols_ + col; }
};
