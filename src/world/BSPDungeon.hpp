#pragma once

#include "Map.hpp"
#include <vector>
#include <memory>
#include <random>

class BSPDungeon {
public:
    struct Room {
        int x, y, w, h;
        int centerX() const { return x + w / 2; }
        int centerY() const { return y + h / 2; }
    };

    // minRoomSize: minimum side length of a room
    // maxDepth:    how many times to split (more = more rooms, smaller)
    BSPDungeon(int mapWidth, int mapHeight, int minRoomSize = 5, int maxDepth = 5);

    // Fills the map with walls, then carves rooms and corridors.
    void generate(Map& map, unsigned int seed = 0);

    const std::vector<Room>& getRooms() const { return rooms_; }

    // First room placed — use as player spawn.
    Room startRoom() const { return rooms_.front(); }

private:
    struct Node {
        int x, y, w, h;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        bool isLeaf() const { return !left && !right; }
    };

    struct Point { int x, y; };

    int mapWidth_, mapHeight_;
    int minRoomSize_, maxDepth_;
    std::vector<Room> rooms_;
    std::mt19937 rng_;

    std::unique_ptr<Node> buildTree(int x, int y, int w, int h, int depth);
    // Returns a connection point for corridor routing. Carves rooms and corridors.
    Point carveNode(Node& node, Map& map);
    void carveRoom(const Room& room, Map& map);
    void carveCorridor(int x1, int y1, int x2, int y2, Map& map);

    int randInt(int lo, int hi); // inclusive
};
