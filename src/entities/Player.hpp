#pragma once

// Player extiende Entity con mana, clase, nivel, XP, equipo e inventario.
// El ATK y DEF efectivos se calculan como base + bonus del ítem equipado;
// se actualizan en levelUp() y cada vez que se equipa/desequipa algo.

#include "Entity.hpp"
#include "inventory/Inventory.hpp"
#include "combat/Art.hpp"
#include "SkillTree.hpp"
#include <vector>
#include <optional>

enum class PlayerClass {
    Warrior,   // alta vida y defensa, artes de cuerpo a cuerpo
    Mage,      // baja vida, mucho mana, artes de área y control
    Ranger,    // equilibrado, artes de DoT y múltiples golpes (Arquera en UI)
    Halley,    // clase secreta — Creador del Tenebrarium
    Nato,      // clase secreta — Equilibrio del Tenebrarium
};

class Player : public Entity {
    friend class Game;
    friend class GameSerializer;
public:
    Player(const std::string& name, PlayerClass playerClass);

    PlayerClass getClass() const { return class_; }
    int getLevel() const { return level_; }
    int getXp() const { return xp_; }
    int getXpToNextLevel() const { return xpToNextLevel_; }
    int getMana()         const { return mana_; }
    int getMaxMana()      const { return maxMana_; }
    int getCoins()        const { return coins_; }
    int getDungeonFloor() const { return dungeonFloor_; }
    int getIncursions()   const { return incursions_; }

    Inventory& getInventory() { return inventory_; }
    const Inventory& getInventory() const { return inventory_; }

    const std::optional<Item>& getEquippedWeapon() const { return equippedWeapon_; }
    const std::optional<Item>& getEquippedArmor()  const { return equippedArmor_;  }

    void gainXp(int amount);               // acumula XP; llama levelUp() si supera el umbral
    bool useMana(int amount);             // retorna false y no descuenta si no hay mana suficiente
    void restoreMana(int amount);
    // ── Habilidades: 3 base + las aprendidas en el árbol; se llevan 3 equipadas ──
    static constexpr int kEquippedArtSlots = 3;
    std::vector<Art> getAvailableArts() const;  // las equipadas, en orden de ranura (teclas 1-3)
    std::vector<Art> getKnownArts() const;      // base + árbol, en orden
    bool knowsArt(ArtEffect effect) const;
    bool hasEquippedArt(ArtEffect effect) const;
    bool equipArt(int slot, ArtEffect effect);  // si ya estaba equipada en otra ranura, se intercambian
    void resetEquippedArts();                   // vuelve a las 3 base
    const std::vector<ArtEffect>& getEquippedArts() const { return equippedArts_; }
    int  getArtRank(ArtEffect effect) const;
    bool upgradeArt(ArtEffect effect);          // +1 rango; false si ya está al máximo
    int  artManaCost(ArtEffect effect) const;   // coste con el descuento del rango

    // ── Árbol de habilidades ─────────────────────────────────────────────────
    struct UnlockedSkill {
        int tier;       // índice del tier (0-indexed)
        int choiceIdx;  // qué opción eligió (0 o 1)
    };

    bool hasPendingSkillUnlock() const { return pendingSkillUnlock_; }
    void setPendingSkillUnlock(bool v) { pendingSkillUnlock_ = v; }
    void refreshPendingSkillUnlock();          // recalcula según nivel y tiers ya elegidos
    bool unlockSkill(int tier, int choiceIdx); // false si ya desbloqueado o inválido
    const std::vector<UnlockedSkill>& getUnlockedSkills() const { return unlockedSkills_; }

    void addCoins(int amount);            // puede ser negativo (compra)
    void descendFloor();                  // incrementa dungeonFloor_
    void beginIncursion();                // reinicia la profundidad a 1 y suma una incursión
    bool pickupItem(const Item& item);    // auto-equipa si el slot está libre, si no va a la mochila
    void equipItem(int bagIdx);           // intercambia el ítem de la mochila[bagIdx] con el slot equipado
    void unequipWeapon();                 // devuelve el arma equipada a la mochila
    void unequipArmor();                  // devuelve la armadura equipada a la mochila
    int  useConsumable();                 // usa la primera poción del inventario; retorna HP curado (0 si no hay)
    int  countConsumables() const;
    int  countHpPotions() const;
    int  countManaPotions() const;

    // Entrenamiento del pueblo: mejoras permanentes de la partida
    void trainHealth(int amount);
    void trainAttack(int amount);
    void trainDefense(int amount);
    void trainMana(int amount);
    // Forja del pueblo: +1 al bonus del arma/armadura equipada. Retorna false si no hay.
    bool upgradeEquippedWeapon();
    bool upgradeEquippedArmor();

private:
    PlayerClass class_;
    int level_;
    int xp_;
    int xpToNextLevel_;
    int mana_;
    int maxMana_;
    int coins_;
    int dungeonFloor_;
    int incursions_{0};
    int baseAttack_;
    int baseDefense_;
    std::optional<Item> equippedWeapon_;
    std::optional<Item> equippedArmor_;
    Inventory inventory_;

    std::vector<UnlockedSkill> unlockedSkills_;
    std::vector<ArtEffect>     equippedArts_;
    std::vector<int>           artRanks_ = std::vector<int>(kArtEffectCount, 1);
    Art makeArt(const SkillChoice& sc) const;
    bool pendingSkillUnlock_ = false;

    void levelUp();

    // Base stats per class
    static int baseHp(PlayerClass c);
    static int baseAttack(PlayerClass c);
    static int baseDefense(PlayerClass c);
    static int baseMana(PlayerClass c);
};
