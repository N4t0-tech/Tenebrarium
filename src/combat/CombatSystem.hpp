#pragma once

// CombatSystem — maneja un encuentro completo por turnos.
// Turno del jugador: el jugador gasta PA (máx 3) con acciones hasta quedarse sin PA o usar doEndTurn().
// Turno del enemigo: se resuelve automáticamente cuando el jugador se queda sin PA.
// Los StatusEffects se aplican al inicio del turno enemigo (tickPlayerEffects/tickEnemyEffects).
//
// Para añadir una nueva acción: agregar método público + lógica en el .cpp.
// Para añadir un nuevo efecto de estado: agregar valor en StatusEffect::Type, manejar en
// tickPlayerEffects/tickEnemyEffects y resolveArt.

#include "Art.hpp"
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include <vector>
#include <string>
#include <memory>

struct StatusEffect {
    enum class Type {
        Poisoned,       // DoT al inicio del turno enemigo (daño por turnsLeft turnos)
        Frozen,         // enemigo pierde 1 PA en su turno (1 turno)
        AttackBoosted,  // jugador tiene ATK extra por turnsLeft turnos
        Defending,      // jugador recibe 50% menos daño durante 1 turno enemigo
        DefendingHeavy, // jugador recibe 70% menos daño en el PRIMER golpe y se consume
        TrapPending,    // enemigo recibe daño retardado al inicio del turno enemigo
    };
    Type type;
    int  turnsLeft;
    int  magnitude;   // cantidad de daño/bonus según el tipo
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

    bool isPlayerAttackBoosted()  const;
    int  getPlayerAttackBoost()   const;
    bool rollCritical()           const;
};
