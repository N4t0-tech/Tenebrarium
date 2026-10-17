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
        // Jugador
        Exposed,        // DEF del jugador -magnitude% (Furia Berserker)
        Regen,          // cura magnitude HP al inicio de cada turno enemigo
        DefenseBoosted, // DEF del jugador +magnitude
        Undying,        // la vida del jugador no baja de 1
        Evading,        // esquiva el próximo golpe y contraataca
        Revive,         // al morir revive con magnitude% de vida (se consume)
        // Enemigo
        Sundered,       // DEF del enemigo -magnitude%
        Weakened,       // ATK del enemigo -magnitude%
        Vulnerable,     // el enemigo recibe +magnitude% de daño
    };
    Type type;
    int  turnsLeft;
    int  magnitude;   // cantidad de daño/bonus según el tipo
};

enum class CombatPhase { PlayerTurn, EnemyTurn, CombatOver };

class CombatSystem {
public:
    // Ningún golpe baja del kMinDamagePct% de su daño bruto, sea cual sea la DEF.
    static constexpr int kMinDamagePct     = 25;
    // Las habilidades ganan un kArtLevelScalePct% de poder por cada nivel del jugador.
    static constexpr int kArtLevelScalePct = 4;
    static int   mitigate(int raw, int def);
    static float artLevelScale(int level);

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
    void doFlee();              // 3 PA — 45% chance to escape
    void doLoot();              // 1 PA — attempt to loot current target, 3 outcomes
    void cycleTarget();         // TAB — cycle to next alive enemy
    int  getLootChance()  const;

private:
    Player&                               player_;
    std::vector<std::unique_ptr<Enemy>>   enemies_;
    std::vector<std::string>              log_;
    int                                   currentTarget_;
    int                                   maxAp_;
    int                                   currentAp_;
    CombatPhase                           phase_;
    bool                                  fled_;
    std::vector<StatusEffect>             playerEffects_;
    std::vector<std::vector<StatusEffect>> enemyEffects_;
    std::vector<int>                      lootAttempts_;
    ArtEffect                             lastArt_{};
    bool                                  hasLastArt_ = false;
    int                                   artRank_    = 1;   // rango del arte que se está resolviendo

    // ── Internal helpers ─────────────────────────────────────────────────────
    bool hasEnoughAp(int amount) const { return currentAp_ >= amount; }
    void logMessage(const std::string& msg);
    void checkCombatOver();
    void advanceTarget();               // move currentTarget_ to next alive enemy
    void doAttackBase(float atkMultiplier, int apCost, int manaCost, const std::string& actionName);

    void processEnemyTurn();
    void tickPlayerEffects();
    void tickEnemyEffects();
    bool resolveArt(ArtEffect effect);  // false si no tuvo efecto: no se cobra PA ni MP

    int  enemyDefense(int idx) const;   // DEF con Rompecorazas/Colapso aplicados
    int  enemyAttack(int idx)  const;   // ATK con Colapso aplicado
    int  playerDefense()       const;   // DEF con Berserker/Bendición aplicados
    int  hitEnemy(int idx, int dmg);    // aplica Vulnerable; devuelve el daño final
    void hurtPlayer(int dmg);           // respeta Último Bastión y Génesis
    void addEnemyEffect(int idx, StatusEffect::Type type, int turns, int magnitude);
    void addPlayerEffect(StatusEffect::Type type, int turns, int magnitude);
    bool playerHas(StatusEffect::Type type) const;
    void handleSlimeSplits();

    bool isPlayerAttackBoosted()  const;
    int  getPlayerAttackBoost()   const;
    int  getEffectiveAttack()     const;
    int  artPower()               const;
    int  ranked(int value)        const;   // aplica el bonus del rango del arte actual
    bool rollCritical()           const;
};
