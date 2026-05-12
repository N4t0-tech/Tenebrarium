#include "Enemy.hpp"

Enemy::Enemy(const std::string& name, int maxHp, int attack, int defense,
             int xpReward, EnemyType type, int basePa,
             bool splitsOnDeath)
    : Entity(name, maxHp, attack, defense),
      xpReward_(xpReward),
      type_(type),
      basePa_(basePa),
      splitsOnDeath_(splitsOnDeath)
{}
