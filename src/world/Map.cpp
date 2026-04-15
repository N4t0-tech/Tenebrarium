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
    return t.type != TileType::Wall
        && t.type != TileType::SecretWall;
}

bool Map::isSecretWall(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
    return tiles_[index(x, y)].type == TileType::SecretWall;
}

void Map::revealSecretWall(int x, int y) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    auto& tile = tiles_[index(x, y)];
    if (tile.type == TileType::SecretWall) {
        tile.type  = TileType::Floor;
        tile.glyph = '.';
    }
}

void Map::setPlayerPos(int x, int y) {
    playerPos_ = {x, y};
}

void Map::updateFov(int radius) {
    // LOS-based FOV: cast a Bresenham ray to every tile within radius.
    // Walls/SecretWalls are opaque — they are visible themselves but block tiles beyond.
    for (auto& t : tiles_) t.visible = false;

    int px = playerPos_.x, py = playerPos_.y;
    if (px < 0 || px >= width_ || py < 0 || py >= height_) return;

    tiles_[index(px, py)].visible  = true;
    tiles_[index(px, py)].explored = true;

    auto isOpaque = [&](int x, int y) -> bool {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return true;
        TileType tt = tiles_[index(x, y)].type;
        return tt == TileType::Wall || tt == TileType::SecretWall;
    };

    for (int ty = std::max(0, py - radius); ty <= std::min(height_ - 1, py + radius); ty++) {
        for (int tx = std::max(0, px - radius); tx <= std::min(width_ - 1, px + radius); tx++) {
            if (tx == px && ty == py) continue;
            int dx = tx - px, dy = ty - py;
            if (dx*dx + dy*dy > radius*radius) continue;

            // Bresenham ray from player toward (tx, ty)
            int x = px, y = py;
            int adx = std::abs(dx), ady = std::abs(dy);
            int sx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
            int sy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
            int err = adx - ady;
            bool blocked = false;

            while (x != tx || y != ty) {
                int e2 = 2 * err;
                if (e2 > -ady) { err -= ady; x += sx; }
                if (e2 <  adx) { err += adx; y += sy; }
                if (x < 0 || x >= width_ || y < 0 || y >= height_) { blocked = true; break; }
                if (x == tx && y == ty) break;  // reached destination — check after loop
                if (isOpaque(x, y)) {
                    // This tile (a wall face) is visible but blocks further vision
                    tiles_[index(x, y)].visible  = true;
                    tiles_[index(x, y)].explored = true;
                    blocked = true;
                    break;
                }
            }

            if (!blocked) {
                tiles_[index(tx, ty)].visible  = true;
                tiles_[index(tx, ty)].explored = true;
            }
        }
    }
}
