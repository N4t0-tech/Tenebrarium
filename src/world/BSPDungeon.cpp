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
        // Place a room randomly within this partition (with 1-tile padding)
        int maxW = std::min(node.w - 2, minRoomSize_ * 2);
        int maxH = std::min(node.h - 2, minRoomSize_ * 2);
        int rw = randInt(minRoomSize_, std::max(minRoomSize_, maxW));
        int rh = randInt(minRoomSize_, std::max(minRoomSize_, maxH));
        int rx = node.x + randInt(1, std::max(1, node.w - rw - 1));
        int ry = node.y + randInt(1, std::max(1, node.h - rh - 1));

        Room room{ rx, ry, rw, rh };
        carveRoom(room, map);
        rooms_.push_back(room);

        return { room.centerX(), room.centerY() };
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

// ─── Util ────────────────────────────────────────────────────────────────────

int BSPDungeon::randInt(int lo, int hi) {
    if (lo >= hi) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(rng_);
}
