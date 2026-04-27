#pragma once

// DungeonPopulator — llena un mapa ya generado con enemigos, cofres, tienda y puntos de interés.
// También es la fábrica de ítems y enemigos usada por la tienda y el combate.
// Todos los métodos son estáticos; no guarda estado.

#include "WorldObjects.hpp"
#include "BSPDungeon.hpp"
#include "../entities/Player.hpp"
#include <vector>
#include <memory>

class DungeonPopulator {
public:
    // Resultado de populate(): todo lo que Game necesita para un piso nuevo.
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

    // Fábrica de ítems — también usada por la tienda y los cofres
    static Item             pickWeapon(PlayerClass cls, int floor);
    static Item             pickArmor(PlayerClass cls, int floor);
    static Item             pickPotion(int floor);

    // Fábrica de enemigos — también usada al entrar en combate
    static EnemyType        pickEnemyType(int floor);
    static std::unique_ptr<Enemy> makeEnemy(EnemyType t, int floor, bool isBoss = false);
    static int              xpForEnemy(EnemyType t, int floor);  // XP base + escalado de piso

private:
    // pickPos: posición aleatoria dentro de la sala evitando las ya ocupadas
    static Position pickPos(const BSPDungeon::Room& r, std::vector<Position>& taken);
    static void tryPlaceSecretRoom(Map& map, std::vector<WorldChest>& chests,
                                   std::vector<Position>& taken, PlayerClass cls, int floor);
};
