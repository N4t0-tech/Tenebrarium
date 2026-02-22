#pragma once

#include <vector>
#include <string>

enum class TileType {
    Floor,
    Wall,
    Door,
    Trap,
    Stairs,
    Empty,
};

struct Tile {
    TileType type;
    char glyph;      // ASCII character to render
    bool explored;
    bool visible;
};

struct Position {
    int x, y;
};

// Represents a dungeon floor or overworld map as a 2D tile grid.
class Map {
public:
    Map(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    bool isWalkable(int x, int y) const;
    void setPlayerPos(int x, int y);
    Position getPlayerPos() const { return playerPos_; }

    // Compute field-of-view from player position with given radius.
    void updateFov(int radius = 8);

private:
    int width_, height_;
    std::vector<Tile> tiles_;
    Position playerPos_{0, 0};

    int index(int x, int y) const { return y * width_ + x; }
};
