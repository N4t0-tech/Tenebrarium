#pragma once

#include "Map.hpp"
#include "../entities/Enemy.hpp"
#include "../inventory/Item.hpp"

enum class ChestLoot { Coins, Item };

struct WorldEnemy {
    Position  pos;
    Position  spawnPos;
    EnemyType type;
    bool      alive;
    bool      isBoss = false;
};

struct WorldChest {
    Position  pos;
    bool      opened;
    ChestLoot loot;
    int       coins;
    Item      item;
};
