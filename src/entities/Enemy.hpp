#pragma once

#include "Entity.hpp"

enum class EnemyType { Goblin, Skeleton, Orc, Spider, Vampire, Zombie, Demon, Shadow };

class Enemy : public Entity {
public:
    Enemy(const std::string& name, int maxHp, int attack, int defense,
          int xpReward, EnemyType type, int basePa = 1);

    int       getXpReward() const { return xpReward_; }
    EnemyType getType()     const { return type_; }
    int       getBasePa()   const { return basePa_; }

private:
    int       xpReward_;
    EnemyType type_;
    int       basePa_;  // number of attacks per turn
};
