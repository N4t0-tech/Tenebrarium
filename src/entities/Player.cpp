#include "Player.hpp"
#include <algorithm>

Player::Player(const std::string& name, PlayerClass playerClass)
    : Entity(name, baseHp(playerClass), baseAttack(playerClass), baseDefense(playerClass)),
      class_(playerClass),
      level_(1),
      xp_(0),
      xpToNextLevel_(100),
      mana_(baseMana(playerClass)),
      maxMana_(baseMana(playerClass)),
      coins_(0),
      keys_(0),
      dungeonFloor_(1),
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

void Player::addCoins(int amount)  { coins_ += amount; }
void Player::addKey()              { keys_++; }
bool Player::useKey()              { if (keys_ <= 0) return false; keys_--; return true; }
void Player::descendFloor()        { dungeonFloor_++; }

void Player::applyItemBonus(const Item& item) {
    if (item.type == ItemType::Weapon) attack_  += item.statBonus;
    else if (item.type == ItemType::Armor)  defense_ += item.statBonus;
    inventory_.addItem(item);
}

bool Player::useMana(int amount) {
    if (mana_ < amount) return false;
    mana_ -= amount;
    return true;
}

void Player::restoreMana(int amount) {
    mana_ = std::min(maxMana_, mana_ + amount);
}

std::vector<Art> Player::getAvailableArts() const {
    switch (class_) {
        case PlayerClass::Warrior:
            return {
                { "Golpe Demoledor", 2, 0,  ArtEffect::GolpeDemoledor, "ATK directo, ignora DEF" },
                { "Grito de Guerra", 1, 10, ArtEffect::GritoDeGuerra,  "+ATK por 2 turnos" },
                { "Escudo Total",    1, 5,  ArtEffect::EscudoTotal,    "-70% proximo golpe recibido" },
            };
        case PlayerClass::Mage:
            return {
                { "Bola de Fuego", 2, 20, ArtEffect::BolaDeFuego, "Dano magico a TODOS" },
                { "Congelar",      1, 10, ArtEffect::Congelar,    "Enemigo pierde 1 PA su turno" },
                { "Drenaje",       3, 30, ArtEffect::Drenaje,     "Dano + recupera HP igual al daño" },
            };
        case PlayerClass::Ranger:
            return {
                { "Disparo Doble", 2, 0, ArtEffect::DisparoDoble, "2 ataques rapidos" },
                { "Trampa",        1, 0, ArtEffect::Trampa,       "Dano retardado turno siguiente" },
                { "Veneno",        1, 0, ArtEffect::Veneno,       "DoT 3 turnos" },
            };
    }
    return {};
}

int Player::baseMana(PlayerClass c) {
    switch (c) {
        case PlayerClass::Warrior: return 20;
        case PlayerClass::Mage:    return 100;
        case PlayerClass::Ranger:  return 50;
    }
    return 30;
}
