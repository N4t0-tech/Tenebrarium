#pragma once

// Enemy representa a un enemigo en combate (instancia temporal creada por DungeonPopulator::makeEnemy).
// No confundir con WorldEnemy (el enemigo en el mapa del mundo, en WorldObjects.hpp).
// Los stats de xpReward que se usan en combate son informativos; el XP real se calcula
// en inputCombat() usando DungeonPopulator::xpForEnemy() para aplicar el escalado por piso.

#include "Entity.hpp"

// Para añadir un tipo nuevo: agregar aquí, en makeEnemy() y en populateFloor() de DungeonPopulator.
enum class EnemyType { Goblin, Skeleton, Orc, Spider, Vampire };

class Enemy : public Entity {
public:
    Enemy(const std::string& name, int maxHp, int attack, int defense,
          int xpReward, EnemyType type, int basePa = 1);

    int       getXpReward() const { return xpReward_; }
    EnemyType getType()     const { return type_; }
    int       getBasePa()   const { return basePa_; }  // ataques por turno (Orco = 2)

private:
    int       xpReward_;
    EnemyType type_;
    int       basePa_;
};
