#pragma once

#include <vector>
#include <string>

enum class TileType {
    Floor,
    Wall,
    SecretWall,  // looks like '#' but is not walkable until revealed via E key
    Stairs,
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

// A renderable entity placed on the map (enemy, chest, door, stairs…)
struct MapEntity {
    Position pos;
    char     glyph;
    int      colorPair;  // ncurses color pair index
    bool     bold = true;
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
    bool isSecretWall(int x, int y) const;
    void revealSecretWall(int x, int y);  // changes SecretWall → Floor
    bool destroyTile(int x, int y);       // destroys wall/secret wall → Floor, returns true if destroyed
    void setPlayerPos(int x, int y);
    Position getPlayerPos() const { return playerPos_; }

    // Compute field-of-view from player position with given radius (LOS ray-cast).
    void updateFov(int radius = 8);

private:
    int width_, height_;
    std::vector<Tile> tiles_;
    Position playerPos_{0, 0};

    int index(int x, int y) const { return y * width_ + x; }
};
