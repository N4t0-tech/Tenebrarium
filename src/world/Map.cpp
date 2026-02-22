#include "Map.hpp"
#include <stdexcept>
#include <cmath>

Map::Map(int width, int height)
    : width_(width), height_(height), tiles_(width * height)
{
    for (auto& tile : tiles_) {
        tile = { TileType::Floor, '.', false, false };
    }
}

Tile& Map::at(int x, int y) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        throw std::out_of_range("Map::at out of bounds");
    return tiles_[index(x, y)];
}

const Tile& Map::at(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        throw std::out_of_range("Map::at out of bounds");
    return tiles_[index(x, y)];
}

bool Map::isWalkable(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
    const Tile& t = tiles_[index(x, y)];
    return t.type != TileType::Wall && t.type != TileType::Empty;
}

void Map::setPlayerPos(int x, int y) {
    playerPos_ = {x, y};
}

void Map::updateFov(int radius) {
    // Mark all tiles as not visible
    for (auto& tile : tiles_) tile.visible = false;

    // Simple circular FOV: reveal tiles within radius of player
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy > radius*radius) continue;
            int nx = playerPos_.x + dx;
            int ny = playerPos_.y + dy;
            if (nx < 0 || nx >= width_ || ny < 0 || ny >= height_) continue;
            tiles_[index(nx, ny)].visible  = true;
            tiles_[index(nx, ny)].explored = true;
        }
    }
}
