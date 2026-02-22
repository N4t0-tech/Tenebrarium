#include "Entity.hpp"
#include <algorithm>

Entity::Entity(const std::string& name, int maxHp, int attack, int defense)
    : name_(name), hp_(maxHp), maxHp_(maxHp), attack_(attack), defense_(defense)
{}

void Entity::takeDamage(int amount) {
    int effective = std::max(0, amount - defense_);
    hp_ = std::max(0, hp_ - effective);
}

void Entity::heal(int amount) {
    hp_ = std::min(maxHp_, hp_ + amount);
}
