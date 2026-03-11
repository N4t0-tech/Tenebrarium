#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <cstdint>

// Flags de celda
static constexpr uint8_t CELL_BOLD     = 1;
static constexpr uint8_t CELL_DIM      = 2;
static constexpr uint8_t CELL_INVERTED = 4;

struct Cell {
    int      codepoint = ' ';
    Color    fg        = WHITE;
    Color    bg        = BLACK;
    uint8_t  flags     = 0;
};

// Buffer de celdas que el Renderer escribe y Raylib dibuja.
class TerminalScreen {
public:
    TerminalScreen(int cols, int rows, int cellW, int cellH, Font font);

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
    // Dibuja el buffer (llamar entre BeginDrawing/EndDrawing)
    void render() const;

    static Color dimColor(Color c);

private:
    int   cols_, rows_, cellW_, cellH_;
    Font  font_;
    std::vector<Cell> buf_;

    bool inBounds(int col, int row) const {
        return col >= 0 && col < cols_ && row >= 0 && row < rows_;
    }
    int idx(int col, int row) const { return row * cols_ + col; }
};
