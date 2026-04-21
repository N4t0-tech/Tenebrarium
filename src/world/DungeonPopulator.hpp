#pragma once

#include "WorldObjects.hpp"
#include "BSPDungeon.hpp"
#include "../entities/Player.hpp"
#include <vector>
#include <memory>

class DungeonPopulator {
public:
    struct Result {
        std::vector<WorldEnemy>  enemies;
        std::vector<WorldChest>  chests;
        Position                 stairsPos{};
        Position                 lockedDoorPos{};
        bool                     lockedDoorExists{false};
        bool                     shopExists{false};
        BSPDungeon::Room         shopRoom{};
        Position                 shopMerchantPos{};
    };

    static Result populate(Map& map, const std::vector<BSPDungeon::Room>& rooms,
                           int floor, PlayerClass cls);

    static Item             pickWeapon(PlayerClass cls, int floor);
    static Item             pickArmor(PlayerClass cls, int floor);
    static Item             pickPotion(int floor);
    static EnemyType        pickEnemyType(int floor);
    static std::unique_ptr<Enemy> makeEnemy(EnemyType t, int floor, bool isBoss = false);
    static int              xpForEnemy(EnemyType t, int floor);

private:
    static Position pickPos(const BSPDungeon::Room& r, std::vector<Position>& taken);
    static void tryPlaceSecretRoom(Map& map, std::vector<WorldChest>& chests,
                                   std::vector<Position>& taken, PlayerClass cls, int floor);
};
