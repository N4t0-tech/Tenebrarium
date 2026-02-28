#pragma once

#include "Entity.hpp"
#include "inventory/Inventory.hpp"
#include "combat/Art.hpp"
#include <vector>
#include <optional>

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
    int getMana()         const { return mana_; }
    int getMaxMana()      const { return maxMana_; }
    int getCoins()        const { return coins_; }
    int getKeys()         const { return keys_; }
    int getDungeonFloor() const { return dungeonFloor_; }

    Inventory& getInventory() { return inventory_; }
    const Inventory& getInventory() const { return inventory_; }

    const std::optional<Item>& getEquippedWeapon() const { return equippedWeapon_; }
    const std::optional<Item>& getEquippedArmor()  const { return equippedArmor_;  }

    void gainXp(int amount);
    bool useMana(int amount);
    void restoreMana(int amount);
    std::vector<Art> getAvailableArts() const;

    void addCoins(int amount);
    void addKey();
    bool useKey();                       // false if no keys
    void descendFloor();
    void applyItemBonus(const Item& item); // delegates to pickupItem
    bool pickupItem(const Item& item);    // auto-equip if slot free, else bag
    void equipItem(int bagIdx);           // swap bag item into equipment slot
    int  useConsumable();                // uses first potion; returns HP healed, 0 if none
    int  countConsumables() const;

private:
    PlayerClass class_;
    int level_;
    int xp_;
    int xpToNextLevel_;
    int mana_;
    int maxMana_;
    int coins_;
    int keys_;
    int dungeonFloor_;
    int baseAttack_;
    int baseDefense_;
    std::optional<Item> equippedWeapon_;
    std::optional<Item> equippedArmor_;
    Inventory inventory_;

    void levelUp();

    // Base stats per class
    static int baseHp(PlayerClass c);
    static int baseAttack(PlayerClass c);
    static int baseDefense(PlayerClass c);
    static int baseMana(PlayerClass c);
};
