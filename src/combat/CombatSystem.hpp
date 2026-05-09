#pragma once

#include "Art.hpp"
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include <vector>
#include <string>
#include <memory>

struct StatusEffect {
    enum class Type {
        Poisoned,       // DoT applied to enemy each turn
        Frozen,         // Enemy loses 1 PA this turn
        AttackBoosted,  // Player ATK increased
        Defending,      // Player takes 50% less damage this enemy turn
        DefendingHeavy, // Player takes 70% less damage on next hit (consumed)
        TrapPending,    // Enemy takes delayed damage on tick
    };
    Type type;
    int  turnsLeft;
    int  magnitude;
};

enum class CombatPhase { PlayerTurn, EnemyTurn, CombatOver };

class CombatSystem {
public:
    CombatSystem(Player& player, std::vector<std::unique_ptr<Enemy>> enemies);

    // ── State queries ────────────────────────────────────────────────────────
    bool        isOver()         const { return phase_ == CombatPhase::CombatOver; }
    bool        playerWon()      const;
    bool        playerFled()     const { return fled_; }
    int         getCurrentAp()   const { return currentAp_; }
    int         getMaxAp()       const { return maxAp_; }
    int         getCurrentTarget() const { return currentTarget_; }
    CombatPhase getPhase()       const { return phase_; }

    const std::vector<std::unique_ptr<Enemy>>& getEnemies()      const { return enemies_; }
    const std::vector<std::string>&            getLog()          const { return log_; }
    const std::vector<StatusEffect>&           getPlayerEffects() const { return playerEffects_; }
    const std::vector<StatusEffect>&           getEnemyEffects(int idx) const;

    // ── Player actions ───────────────────────────────────────────────────────
    void doAttack();            // 1 PA — ATK - DEF, min 1
    void doHeavyAttack();       // 2 PA — ATK*1.8 - DEF
    void doArt(int idx);        // variable PA — resolves art by ArtEffect
    void doDefend();            // 1 PA — Defending status for 1 enemy turn
    void doUseItem();
    void doEndTurn();           // 0 PA — force enemy turn
    void doFlee();              // 3 PA — 70% chance to escape
    void doLoot();              // 1 PA — attempt to loot current target, 3 outcomes
    void cycleTarget();         // TAB — cycle to next alive enemy

private:
    Player&                               player_;
    std::vector<std::unique_ptr<Enemy>>   enemies_;
    std::vector<std::string>              log_;
    int                                   currentTarget_;
    int                                   currentAp_;
    static constexpr int                  maxAp_ = 3;
    CombatPhase                           phase_;
    bool                                  fled_;
    std::vector<StatusEffect>             playerEffects_;
    std::vector<std::vector<StatusEffect>> enemyEffects_;

    // ── Internal helpers ─────────────────────────────────────────────────────
    bool hasEnoughAp(int amount) const { return currentAp_ >= amount; }
    void logMessage(const std::string& msg);
    void checkCombatOver();
    void advanceTarget();               // move currentTarget_ to next alive enemy
    void doAttackBase(float atkMultiplier, int apCost, int manaCost, const std::string& actionName);

    void processEnemyTurn();
    void tickPlayerEffects();
    void tickEnemyEffects();
    void resolveArt(ArtEffect effect);
    void handleSlimeSplits();

    bool isPlayerAttackBoosted()  const;
    int  getPlayerAttackBoost()   const;
    int  getEffectiveAttack()     const;
    bool rollCritical()           const;
};
