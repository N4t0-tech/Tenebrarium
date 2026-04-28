#pragma once

// Player extiende Entity con mana, clase, nivel, XP, equipo e inventario.
// El ATK y DEF efectivos se calculan como base + bonus del ítem equipado;
// se actualizan en levelUp() y cada vez que se equipa/desequipa algo.

#include "Entity.hpp"
#include "inventory/Inventory.hpp"
#include "combat/Art.hpp"
#include <vector>
#include <optional>

enum class PlayerClass {
    Warrior,  // alta vida y defensa, artes de cuerpo a cuerpo
    Mage,     // baja vida, mucho mana, artes de área y control
    Ranger,   // equilibrado, artes de DoT y múltiples golpes
};

class Player : public Entity {
    friend class Game;
    friend class GameSerializer;
public:
    Player(const std::string& name, PlayerClass playerClass);

    PlayerClass getClass() const { return class_; }
    int getLevel() const { return level_; }
    int getXp() const { return xp_; }
    int getMana()         const { return mana_; }
    int getMaxMana()      const { return maxMana_; }
    int getCoins()        const { return coins_; }
    int getDungeonFloor() const { return dungeonFloor_; }

    Inventory& getInventory() { return inventory_; }
    const Inventory& getInventory() const { return inventory_; }

    const std::optional<Item>& getEquippedWeapon() const { return equippedWeapon_; }
    const std::optional<Item>& getEquippedArmor()  const { return equippedArmor_;  }

    void gainXp(int amount);               // acumula XP; llama levelUp() si supera el umbral
    bool useMana(int amount);             // retorna false y no descuenta si no hay mana suficiente
    void restoreMana(int amount);
    std::vector<Art> getAvailableArts() const;  // lista de habilidades según la clase

    void addCoins(int amount);            // puede ser negativo (compra)
    void descendFloor();                  // incrementa dungeonFloor_
    bool pickupItem(const Item& item);    // auto-equipa si el slot está libre, si no va a la mochila
    void equipItem(int bagIdx);           // intercambia el ítem de la mochila[bagIdx] con el slot equipado
    void unequipWeapon();                 // devuelve el arma equipada a la mochila
    void unequipArmor();                  // devuelve la armadura equipada a la mochila
    int  useConsumable();                 // usa la primera poción del inventario; retorna HP curado (0 si no hay)
    int  countConsumables() const;

private:
    PlayerClass class_;
    int level_;
    int xp_;
    int xpToNextLevel_;
    int mana_;
    int maxMana_;
    int coins_;
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
