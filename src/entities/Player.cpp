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
      dungeonFloor_(1),
      baseAttack_(baseAttack(playerClass)),
      baseDefense_(baseDefense(playerClass)),
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
    xpToNextLevel_ = static_cast<int>(xpToNextLevel_ * 1.5);  // umbral crece x1.5 cada nivel
    maxHp_ += 10;
    hp_ = maxHp_;   // curación completa al subir nivel
    baseAttack_  += 2;
    baseDefense_ += 1;
    // Recalcular ATK/DEF efectivo incluyendo el bonus del equipo actual
    attack_  = baseAttack_  + (equippedWeapon_ ? equippedWeapon_->statBonus : 0);
    defense_ = baseDefense_ + (equippedArmor_  ? equippedArmor_->statBonus  : 0);
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

void Player::descendFloor()        { dungeonFloor_++; }



bool Player::pickupItem(const Item& item) {
    if (item.type == ItemType::Weapon && !equippedWeapon_) {
        equippedWeapon_ = item;
        attack_ = baseAttack_ + item.statBonus;
        return true;
    }
    if (item.type == ItemType::Armor && !equippedArmor_) {
        equippedArmor_ = item;
        defense_ = baseDefense_ + item.statBonus;
        return true;
    }
    return inventory_.addItem(item);
}

void Player::equipItem(int idx) {
    const auto& bag = inventory_.items();
    if (idx < 0 || idx >= static_cast<int>(bag.size())) return;
    Item chosen = bag[idx];  // copy before mutating
    if (chosen.type == ItemType::Weapon) {
        inventory_.removeItem(chosen.name);
        if (equippedWeapon_) inventory_.addItem(*equippedWeapon_);
        equippedWeapon_ = chosen;
        attack_ = baseAttack_ + chosen.statBonus;
    } else if (chosen.type == ItemType::Armor) {
        inventory_.removeItem(chosen.name);
        if (equippedArmor_) inventory_.addItem(*equippedArmor_);
        equippedArmor_ = chosen;
        defense_ = baseDefense_ + chosen.statBonus;
    }
}

int Player::useConsumable() {
    for (const auto& item : inventory_.items()) {
        if (item.type == ItemType::Consumable) {
            int healed = std::min(item.statBonus, maxHp_ - hp_);
            hp_ += healed;
            inventory_.removeItem(item.name);
            return healed;
        }
    }
    return 0;
}

int Player::countConsumables() const {
    int count = 0;
    for (const auto& item : inventory_.items())
        if (item.type == ItemType::Consumable) count++;
    return count;
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
                { "Bola de Fuego", 2, 20, ArtEffect::BolaDeFuego, "Daño mágico a TODOS" },
                { "Congelar",      1, 10, ArtEffect::Congelar,    "Enemigo pierde 1 PA su turno" },
                { "Drenaje",       3, 30, ArtEffect::Drenaje,     "Daño + recupera HP igual al daño" },
            };
        case PlayerClass::Ranger:
            return {
                { "Disparo Doble", 2, 0, ArtEffect::DisparoDoble, "2 ataques rapidos" },
                { "Trampa",        1, 0, ArtEffect::Trampa,       "Daño retardado turno siguiente" },
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
