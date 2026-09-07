#pragma once

#include "WorldObjects.hpp"
#include "BSPDungeon.hpp"
#include "../entities/Player.hpp"
#include <vector>
#include <memory>
#include <random>

struct SecretCand { int wallX, wallY, dx, dy; };

class DungeonPopulator {
public:
    struct Result {
        std::vector<WorldEnemy>  enemies;
        std::vector<WorldChest>  chests;
        std::vector<WorldTorch>  torches;
        Position                 stairsPos{};
        Position                 stairsUpPos{};
        bool                     stairsUpExists{false};
        Position                 lockedDoorPos{};
        bool                     lockedDoorExists{false};
    };

    // Multiplicador global de dificultad: combina la profundidad dentro de la
    // incursión con el número de incursión (cada bajada suma un peldaño).
    static float difficultyFactor(int depth, int incursions);

    static Result populate(Map& map, const std::vector<BSPDungeon::Room>& rooms,
                           int depth, PlayerClass cls, int incursions, std::mt19937& rng);

    static Item             pickWeapon(PlayerClass cls, int floor);
    static Item             pickArmor(PlayerClass cls, int floor);
    static Item             pickPotion(int floor);
    static Item             pickBeer();
    static Item             pickManaPotion();
    static Item             pickBomb(int floor);
    static Item             pickShovel(int floor);
    static EnemyType        pickEnemyType(int floor, std::mt19937& rng);
    static std::unique_ptr<Enemy> makeEnemy(EnemyType t, int depth, int incursions, bool isBoss = false);
    static int              xpForEnemy(EnemyType t, int depth, int incursions);

private:
    static Position pickPos(const BSPDungeon::Room& r, std::vector<Position>& taken,
                            std::mt19937& rng);
    static void tryPlaceSecretRoom(Map& map, std::vector<WorldChest>& chests,
                                   std::vector<Position>& taken, PlayerClass cls, int floor,
                                   std::vector<SecretCand>& cands, std::mt19937& rng);
};
