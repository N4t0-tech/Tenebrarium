// BSPDungeon — generación procedural de mazmorras usando Binary Space Partitioning.
// Algoritmo:
//   1. buildTree(): divide recursivamente el espacio en rectángulos (nodos BSP).
//      Se para cuando profundidad >= maxDepth_ o el espacio es muy pequeño.
//   2. carveNode(): recorre el árbol en post-order. En las hojas coloca una sala
//      aleatoria. En los nodos internos conecta los centros de los hijos con un
//      corredor en L (horizontal→vertical o vertical→horizontal al azar).
//   3. carveNode() retorna un punto (centro de sala) para que el padre pueda
//      conectar los dos subárboles.

#include "BSPDungeon.hpp"
#include <algorithm>

BSPDungeon::BSPDungeon(int mapWidth, int mapHeight, int minRoomSize, int maxDepth)
    : mapWidth_(mapWidth), mapHeight_(mapHeight),
      minRoomSize_(minRoomSize), maxDepth_(maxDepth)
{}

void BSPDungeon::generate(Map& map, unsigned int seed) {
    rng_.seed(seed ? seed : std::random_device{}());
    rooms_.clear();

    // Fill map with walls
    for (int y = 0; y < map.height(); y++)
        for (int x = 0; x < map.width(); x++)
            map.at(x, y) = { TileType::Wall, '#', false, false };

    auto root = buildTree(1, 1, map.width() - 2, map.height() - 2, 0);
    carveNode(*root, map);
    thinCorridors(map);
}

// ─── Tree construction ───────────────────────────────────────────────────────

std::unique_ptr<BSPDungeon::Node> BSPDungeon::buildTree(int x, int y, int w, int h, int depth) {
    auto node = std::make_unique<Node>();
    node->x = x; node->y = y; node->w = w; node->h = h;

    // Stop splitting if we're too deep or the partition is too small to split
    bool tooSmallH = w < minRoomSize_ * 2 + 1;
    bool tooSmallV = h < minRoomSize_ * 2 + 1;
    if (depth >= maxDepth_ || (tooSmallH && tooSmallV))
        return node; // leaf

    // Choose split direction: prefer splitting along the longer axis
    bool splitVertical;
    if (tooSmallH)        splitVertical = false;
    else if (tooSmallV)   splitVertical = true;
    else                  splitVertical = (rng_() % 2 == 0);

    if (splitVertical) {
        // Split left/right: choose x split point
        int lo = minRoomSize_;
        int hi = w - minRoomSize_ - 1;
        if (lo >= hi) return node; // can't split
        int split = randInt(lo, hi);
        node->left  = buildTree(x,          y, split,         h, depth + 1);
        node->right = buildTree(x + split,  y, w - split,     h, depth + 1);
    } else {
        // Split top/bottom
        int lo = minRoomSize_;
        int hi = h - minRoomSize_ - 1;
        if (lo >= hi) return node;
        int split = randInt(lo, hi);
        node->left  = buildTree(x, y,           w, split,     depth + 1);
        node->right = buildTree(x, y + split,   w, h - split, depth + 1);
    }

    return node;
}

// ─── Carving ─────────────────────────────────────────────────────────────────

BSPDungeon::Point BSPDungeon::carveNode(Node& node, Map& map) {
    if (node.isLeaf()) {
        int maxW = std::min(node.w - 2, minRoomSize_ * 2);
        int maxH = std::min(node.h - 2, minRoomSize_ * 2);
        if (maxW < 1 || maxH < 1)
            return { node.x + node.w / 2, node.y + node.h / 2 };

        int rw = randInt(std::min(minRoomSize_, maxW), maxW);
        int rh = randInt(std::min(minRoomSize_, maxH), maxH);

        // Use 2-tile padding when possible, else fall back to 1
        int padX = (node.w - rw >= 4) ? 2 : 1;
        int padY = (node.h - rh >= 4) ? 2 : 1;
        int rx = node.x + randInt(padX, std::max(padX, node.w - rw - padX));
        int ry = node.y + randInt(padY, std::max(padY, node.h - rh - padY));

        Room room{ rx, ry, rw, rh };
        carveRoom(room, map);
        rooms_.push_back(room);

        // Return a point on the room edge facing the sibling
        int edge = randInt(0, 3);
        switch (edge) {
            case 0: return { room.x + randInt(1, room.w - 2), room.y };
            case 1: return { room.x + randInt(1, room.w - 2), room.y + room.h - 1 };
            case 2: return { room.x, room.y + randInt(1, room.h - 2) };
            default: return { room.x + room.w - 1, room.y + randInt(1, room.h - 2) };
        }
    }

    Point leftPt  = carveNode(*node.left,  map);
    Point rightPt = carveNode(*node.right, map);

    // Connect the two subtrees with an L-shaped corridor
    carveCorridor(leftPt.x, leftPt.y, rightPt.x, rightPt.y, map);

    // Pass up one of the connection points at random
    return (rng_() % 2) ? leftPt : rightPt;
}

void BSPDungeon::carveRoom(const Room& room, Map& map) {
    for (int y = room.y; y < room.y + room.h; y++)
        for (int x = room.x; x < room.x + room.w; x++)
            map.at(x, y) = { TileType::Floor, '.', false, false };
}

void BSPDungeon::carveCorridor(int x1, int y1, int x2, int y2, Map& map) {
    // L-shape: horizontal then vertical (or vice versa, random)
    auto carveFloor = [&](int x, int y) {
        if (x >= 0 && x < map.width() && y >= 0 && y < map.height())
            map.at(x, y) = { TileType::Floor, '.', false, false };
    };

    if (rng_() % 2 == 0) {
        // Horizontal first
        int stepX = (x2 > x1) ? 1 : -1;
        for (int x = x1; x != x2; x += stepX) carveFloor(x, y1);
        int stepY = (y2 > y1) ? 1 : -1;
        for (int y = y1; y != y2 + stepY; y += stepY) carveFloor(x2, y);
    } else {
        // Vertical first
        int stepY = (y2 > y1) ? 1 : -1;
        for (int y = y1; y != y2; y += stepY) carveFloor(x1, y);
        int stepX = (x2 > x1) ? 1 : -1;
        for (int x = x1; x != x2 + stepX; x += stepX) carveFloor(x, y2);
    }
}

// ─── Corridor thinning ────────────────────────────────────────────────────────

static int floorNeighbors(Map& map, int x, int y) {
    int n = 0;
    if (map.at(x - 1, y).type == TileType::Floor) n++;
    if (map.at(x + 1, y).type == TileType::Floor) n++;
    if (map.at(x, y - 1).type == TileType::Floor) n++;
    if (map.at(x, y + 1).type == TileType::Floor) n++;
    return n;
}

void BSPDungeon::thinCorridors(Map& map) {
    int w = map.width();
    int h = map.height();
    std::vector<bool> isRoom(w * h, false);
    for (auto& r : rooms_)
        for (int y = r.y; y < r.y + r.h; y++)
            for (int x = r.x; x < r.x + r.w; x++)
                isRoom[y * w + x] = true;

    // Walls adjacent to corridor ends often miss a tile — fill them in
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            if (map.at(x, y).type != TileType::Floor) continue;
            if (isRoom[y * w + x]) continue;
            int nf = floorNeighbors(map, x, y);
            // Dead-end corridor tile with only 1 floor neighbor →
            // fill in walls to make the dead end feel tighter
            if (nf == 1) {
                for (auto [dx, dy] : {std::pair{-1,0},{1,0},{0,-1},{0,1}}) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h &&
                        map.at(nx, ny).type == TileType::Wall)
                        map.at(nx, ny).glyph = '#';
                }
            }
        }
    }

    // Horizontal corridor thinning: collapse 2-tile-high sections to 1 tile
    for (int y = 1; y < h - 2; y++) {
        for (int x = 1; x < w - 1; x++) {
            if (isRoom[y * w + x] || isRoom[(y + 1) * w + x]) continue;
            if (map.at(x, y).type != TileType::Floor) continue;
            if (map.at(x, y + 1).type != TileType::Floor) continue;
            if (map.at(x, y - 1).type == TileType::Wall &&
                map.at(x, y + 2).type == TileType::Wall)
                map.at(x, y + 1) = { TileType::Wall, '#', false, false };
        }
    }
    // Vertical corridor thinning: collapse 2-tile-wide sections to 1 tile
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 2; x++) {
            if (isRoom[y * w + x] || isRoom[y * w + x + 1]) continue;
            if (map.at(x, y).type != TileType::Floor) continue;
            if (map.at(x + 1, y).type != TileType::Floor) continue;
            if (map.at(x - 1, y).type == TileType::Wall &&
                map.at(x + 2, y).type == TileType::Wall)
                map.at(x + 1, y) = { TileType::Wall, '#', false, false };
        }
    }
}

// ─── Util ────────────────────────────────────────────────────────────────────

int BSPDungeon::randInt(int lo, int hi) {
    if (lo >= hi) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(rng_);
}
