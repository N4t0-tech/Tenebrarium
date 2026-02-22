#pragma once

#include "Entity.hpp"
#include "inventory/Inventory.hpp"

enum class PlayerClass {
    Warrior,
    Mage,
    Ranger,
};

class Player : public Entity {
public:
    Player(const std::string& name, PlayerClass playerClass);

    PlayerClass getClass() const { return class_; }
    int getLevel() const { return level_; }
    int getXp() const { return xp_; }
    int getMana() const { return mana_; }
    int getMaxMana() const { return maxMana_; }

    Inventory& getInventory() { return inventory_; }
    const Inventory& getInventory() const { return inventory_; }

    void gainXp(int amount);

private:
    PlayerClass class_;
    int level_;
    int xp_;
    int xpToNextLevel_;
    int mana_;
    int maxMana_;
    Inventory inventory_;

    void levelUp();

    // Base stats per class
    static int baseHp(PlayerClass c);
    static int baseAttack(PlayerClass c);
    static int baseDefense(PlayerClass c);
    static int baseMana(PlayerClass c);
};
