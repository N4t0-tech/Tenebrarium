#pragma once

// Objetos que existen en el mapa del mundo (no en combate).
// WorldEnemy es una representación ligera del enemigo en el mapa; cuando se activa
// combate se crea un Enemy real mediante DungeonPopulator::makeEnemy().
// spawnPos se usa para que la IA regrese al origen cuando el jugador está en la tienda.

#include "Map.hpp"
#include "../entities/Enemy.hpp"
#include "../inventory/Item.hpp"

enum class ChestLoot { Coins, Item };

struct WorldEnemy {
    Position  pos;        // posición actual en el mapa
    Position  spawnPos;   // posición original de aparición (retorno cuando jugador está en tienda)
    EnemyType type;
    bool      alive;      // false = eliminado, no se dibuja ni mueve
    bool      isBoss = false;
    int       remainingHp{-1};  // -1 = HP completo; >0 = HP guardado al huir del combate
};

struct WorldChest {
    Position  pos;
    bool      opened;     // true = ya recogido, no se dibuja
    ChestLoot loot;
    int       coins;
    Item      item;
    bool      isMimic = false;
};
