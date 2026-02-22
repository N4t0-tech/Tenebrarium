#include "Player.hpp"

Player::Player(const std::string& name, PlayerClass playerClass)
    : Entity(name, baseHp(playerClass), baseAttack(playerClass), baseDefense(playerClass)),
      class_(playerClass),
      level_(1),
      xp_(0),
      xpToNextLevel_(100),
      mana_(baseMana(playerClass)),
      maxMana_(baseMana(playerClass)),
      inventory_(20)
{}

void Player::gainXp(int amount) {
    xp_ += amount;
    while (xp_ >= xpToNextLevel_) {
        xp_ -= xpToNextLevel_;
        levelUp();
    }
}

void Player::levelUp() {
    level_++;
    xpToNextLevel_ = static_cast<int>(xpToNextLevel_ * 1.5);
    maxHp_ += 10;
    hp_ = maxHp_;
    attack_ += 2;
    defense_ += 1;
    maxMana_ += 5;
    mana_ = maxMana_;
}

int Player::baseHp(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return 120;
        case PlayerClass::Mage:    return 70;
        case PlayerClass::Ranger:  return 90;
    }
    return 100;
}

int Player::baseAttack(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return 15;
        case PlayerClass::Mage:    return 8;
        case PlayerClass::Ranger:  return 12;
    }
    return 10;
}

int Player::baseDefense(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return 8;
        case PlayerClass::Mage:    return 3;
        case PlayerClass::Ranger:  return 5;
    }
    return 5;
}

int Player::baseMana(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return 20;
        case PlayerClass::Mage:    return 100;
        case PlayerClass::Ranger:  return 50;
    }
    return 30;
}
