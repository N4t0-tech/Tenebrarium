#include "CombatSystem.hpp"
#include "../world/DungeonPopulator.hpp"
#include "audio/Sfx.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>

// ─── helpers ─────────────────────────────────────────────────────────────────

static std::string ts(int n) { return std::to_string(n); }

// ─── constructor ─────────────────────────────────────────────────────────────

CombatSystem::CombatSystem(Player& player, std::vector<std::unique_ptr<Enemy>> enemies)
    : player_(player),
      enemies_(std::move(enemies)),
      currentTarget_(0),
      maxAp_(player.getClass() == PlayerClass::Ranger ? 4 : 3),
      currentAp_(maxAp_),
      phase_(CombatPhase::PlayerTurn),
      fled_(false)
{
    enemyEffects_.resize(enemies_.size());
    lootAttempts_.resize(enemies_.size(), 0);
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    logMessage("=== Comienza el combate! ===");
    logMessage("PA disponibles: " + ts(currentAp_) + "/" + ts(maxAp_));
}

// ─── state queries ────────────────────────────────────────────────────────────

bool CombatSystem::playerWon() const {
    return player_.isAlive() && !fled_ &&
           std::all_of(enemies_.begin(), enemies_.end(),
               [](const std::unique_ptr<Enemy>& e) { return !e->isAlive(); });
}

const std::vector<StatusEffect>& CombatSystem::getEnemyEffects(int idx) const {
    static const std::vector<StatusEffect> empty;
    if (idx < 0 || idx >= static_cast<int>(enemyEffects_.size())) return empty;
    return enemyEffects_[idx];
}

// ─── player actions ───────────────────────────────────────────────────────────

// doAttackBase — implementación compartida de doAttack y doHeavyAttack.
// Fórmula de daño: max(1, ATK*mult + boost - DEF_enemigo), con crítico ×1.5.
// El orden importa: se aplica el boost antes de restar DEF.
void CombatSystem::doAttackBase(float atkMultiplier, int apCost, int manaCost,
                                const std::string& actionName)
{
    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    auto& target = enemies_[currentTarget_];
    if (!target->isAlive()) { logMessage("No hay objetivos vivos."); return; }

    if (manaCost > 0 && !player_.useMana(manaCost)) {
        logMessage("Mana insuficiente para " + actionName + " (necesitas " + ts(manaCost) + ").");
        return;
    }

    int rawAtk = static_cast<int>(getEffectiveAttack() * atkMultiplier);
    if (isPlayerAttackBoosted()) rawAtk += getPlayerAttackBoost();
    int  dmg  = mitigate(rawAtk, enemyDefense(currentTarget_));
    bool crit = rollCritical();
    if (crit) dmg = dmg * 3 / 2;
    dmg = hitEnemy(currentTarget_, dmg);
    Sfx::play(crit ? SfxId::Crit : SfxId::Hit);

    std::string prefix = crit ? "CRITICO! " : "";
    logMessage(prefix + player_.getName() + " " + actionName + " a " +
               target->getName() + " por " + ts(dmg) + " daño.");
    if (!target->isAlive()) {
        logMessage("  " + target->getName() + " derrotado!");
        advanceTarget();
    }

    currentAp_ -= apCost;
    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doAttack() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes para atacar."); return; }
    doAttackBase(1.0f, 1, 0, "ataca");
}

void CombatSystem::doHeavyAttack() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(2)) { logMessage("PA insuficientes para ataque fuerte (necesitas 2)."); return; }
    doAttackBase(1.8f, 2, 8, "Ataque Fuerte");
}

void CombatSystem::doArt(int idx) {
    if (phase_ != CombatPhase::PlayerTurn) return;
    auto arts = player_.getAvailableArts();
    if (idx < 0 || idx >= static_cast<int>(arts.size())) return;

    const Art& art = arts[idx];
    if (!hasEnoughAp(art.apCost)) {
        logMessage("PA insuficientes para " + art.name + ".");
        return;
    }
    if (!player_.useMana(art.manaCost)) {
        logMessage("MP insuficientes para " + art.name + ".");
        return;
    }

    logMessage(player_.getName() + " usa " + art.name + "!");
    artRank_ = art.rank;
    bool ok  = resolveArt(art.effect);
    artRank_ = 1;
    if (!ok) {
        player_.restoreMana(art.manaCost);
        return;
    }
    Sfx::play(artSelf_ ? SfxId::Buff : SfxId::Spell);
    if (art.effect != ArtEffect::Paradoja) {
        lastArt_    = art.effect;
        hasLastArt_ = true;
    }

    currentAp_ -= art.apCost;
    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doDefend() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes para defender."); return; }

    bool already = std::any_of(playerEffects_.begin(), playerEffects_.end(),
        [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Defending; });
    if (already) {
        logMessage("Ya estás en postura defensiva.");
        return;
    }

    StatusEffect fx;
    fx.type      = StatusEffect::Type::Defending;
    fx.turnsLeft = 1;
    fx.magnitude = 50;
    playerEffects_.push_back(fx);
    logMessage(player_.getName() + " adopta postura defensiva (-50% daño).");
    Sfx::play(SfxId::Defend);

    currentAp_--;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doUseItem() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes."); return; }
    int healed = player_.useConsumable();
    if (healed == 0) {
        if (player_.countConsumables() > 0)
            logMessage("Ya tienes la vida al máximo.");
        else
            logMessage("No tienes pociones.");
        return;
    }
    logMessage(player_.getName() + " usa una poción y recupera " +
               std::to_string(healed) + " HP.");
    Sfx::play(SfxId::Drink);
    currentAp_--;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doEndTurn() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    logMessage(player_.getName() + " termina su turno.");
    currentAp_ = 0;
    processEnemyTurn();
}

void CombatSystem::doFlee() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (std::any_of(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<Enemy>& e) { return e->isAlive() && e->isBoss(); })) {
        logMessage("No puedes huir de un jefe!");
        return;
    }
    if (!hasEnoughAp(3)) {
        logMessage("PA insuficientes para huir (necesitas 3 PA).");
        return;
    }

    currentAp_ -= 3;
    if ((std::rand() % 100) < 45) {
        fled_ = true;
        logMessage(player_.getName() + " huye del combate!");
        Sfx::play(SfxId::Flee);
        phase_ = CombatPhase::CombatOver;
    } else {
        logMessage("!" + player_.getName() + " intenta huir pero falla!");
        Sfx::play(SfxId::FleeFail);
        if (currentAp_ <= 0) processEnemyTurn();
    }
}

void CombatSystem::doLoot() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    auto& target = enemies_[currentTarget_];
    if (!target->isAlive()) { logMessage("No hay objetivos vivos."); return; }
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes para saquear."); return; }
    if (currentTarget_ >= static_cast<int>(lootAttempts_.size())) {
        logMessage("No puedes saquear a este enemigo.");
        return;
    }

    int& attempts = lootAttempts_[currentTarget_];
    if (attempts >= 3) {
        logMessage(target->getName() + " no tiene mas objetos.");
        return;
    }

    currentAp_ -= 1;

    int successThreshold = std::max(0, 40 - attempts * 15);
    int partialThreshold = std::max(0, 70 - attempts * 20);
    attempts++;

    int roll = std::rand() % 100;

    if (roll < successThreshold) {
        Item loot = DungeonPopulator::pickPotion(player_.getDungeonFloor());
        player_.getInventory().addItem(loot);
        logMessage("Saqueo exitoso! Obtienes " + loot.name + ".");
        Sfx::play(SfxId::Coin);
    } else if (roll < partialThreshold) {
        Item loot = DungeonPopulator::pickPotion(player_.getDungeonFloor());
        player_.getInventory().addItem(loot);
        int dmg = std::max(1, enemyAttack(currentTarget_) - playerDefense()) / 2;
        Sfx::play(SfxId::Coin);
        sfxDelay_ = 0.2f;
        hurtPlayer(dmg);
        sfxDelay_ = 0.0f;
        logMessage("Saqueas pero " + target->getName() + " te golpea! Recibes " + ts(dmg) + " daño.");
    } else {
        int dmg = std::max(1, enemyAttack(currentTarget_) - playerDefense());
        hurtPlayer(dmg);
        logMessage("No logras saquear! " + target->getName() + " te golpea por " + ts(dmg) + " daño.");
    }

    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

int CombatSystem::getLootChance() const {
    if (currentTarget_ < 0 || currentTarget_ >= static_cast<int>(enemies_.size()))
        return 0;
    if (!enemies_[currentTarget_]->isAlive()) return 0;
    if (currentTarget_ >= static_cast<int>(lootAttempts_.size())) return 0;
    int a = lootAttempts_[currentTarget_];
    if (a >= 3) return 0;
    return 70 - a * 20; // 70, 50, 30
}

void CombatSystem::cycleTarget() {
    int n = static_cast<int>(enemies_.size());
    if (n == 0) return;
    for (int i = 1; i <= n; i++) {
        int next = (currentTarget_ + i) % n;
        if (enemies_[next]->isAlive()) {
            currentTarget_ = next;
            logMessage("Objetivo: " + enemies_[currentTarget_]->getName());
            return;
        }
    }
}

// ─── internal ────────────────────────────────────────────────────────────────

void CombatSystem::advanceTarget() {
    int n = static_cast<int>(enemies_.size());
    for (int i = 0; i < n; i++) {
        if (enemies_[i]->isAlive()) {
            currentTarget_ = i;
            return;
        }
    }
    currentTarget_ = 0;
}

void CombatSystem::logMessage(const std::string& msg) {
    log_.push_back(msg);
    if (log_.size() > 20) log_.erase(log_.begin());
}

void CombatSystem::handleSlimeSplits() {
    for (int i = 0; i < static_cast<int>(enemies_.size()); i++) {
        if (!enemies_[i]->isAlive() && enemies_[i]->getSplitsOnDeath()) {
            int babyXP = std::max(1, enemies_[i]->getXpReward() / 2);
            int babyHp = std::max(1, enemies_[i]->getMaxHp() / 2);
            int babyAtk = std::max(1, enemies_[i]->getAttack() / 2);
            int babyDef = std::max(1, enemies_[i]->getDefense() / 2);
            logMessage("El Slime se divide en 2!");
            for (int b = 0; b < 2; b++) {
                enemies_.push_back(std::make_unique<Enemy>(
                    "Slime Pequeño", babyHp, babyAtk, babyDef,
                    babyXP, EnemyType::Slime, 1, false));
                enemyEffects_.emplace_back();
                lootAttempts_.push_back(0);
            }
            enemies_[i]->setSplitsOnDeath(false);
        }
    }
}

void CombatSystem::checkCombatOver() {
    if (!player_.isAlive() || fled_) {
        phase_ = CombatPhase::CombatOver;
        return;
    }
    handleSlimeSplits();
    bool allDead = std::all_of(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& e) { return !e->isAlive(); });
    if (allDead) {
        phase_ = CombatPhase::CombatOver;
        logMessage("=== Victoria! Todos los enemigos derrotados. ===");
        Sfx::play(SfxId::Victory, sfxDelay_ + 0.4f);
    }
}

// processEnemyTurn — orden de resolución por turno enemigo:
// 1. tick efectos del jugador (veneno le hace daño, efectos expiran)
// 2. tick efectos de enemigos (veneno/trampa les hace daño, congelado reduce PA)
// 3. verificar fin de combate (veneno puede haber matado a alguien)
// 4. cada enemigo vivo ataca según su PA base
// Los efectos defensivos (DefendingHeavy, Defending) se resuelven por golpe individual.
// DefendingHeavy tiene prioridad sobre Defending y se consume al primer golpe recibido.
void CombatSystem::processEnemyTurn() {
    if (phase_ == CombatPhase::CombatOver) return;
    phase_ = CombatPhase::EnemyTurn;
    logMessage("--- Turno enemigo ---");
    sfxDelay_ = 0.35f;

    tickPlayerEffects();
    tickEnemyEffects();

    checkCombatOver();
    if (isOver()) return;

    for (int i = 0; i < static_cast<int>(enemies_.size()); i++) {
        auto& enemy = enemies_[i];
        if (!enemy->isAlive()) continue;

        int attackCount = enemy->getBasePa();
        int totalFrozen = 0;
        for (const auto& fx : enemyEffects_[i]) {
            if (fx.type == StatusEffect::Type::Frozen)
                totalFrozen += fx.magnitude;
        }
        if (totalFrozen > 0) {
            attackCount = std::max(0, attackCount - totalFrozen);
            logMessage(enemy->getName() + " congelado (-" + ts(totalFrozen) + " PA).");
        }

        for (int pa = 0; pa < attackCount; pa++) {
            if (!player_.isAlive() || !enemy->isAlive()) break;

            // Paso Sombrío: esquiva el golpe y contraataca
            auto evadeIt = std::find_if(playerEffects_.begin(), playerEffects_.end(),
                [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Evading; });
            if (evadeIt != playerEffects_.end()) {
                playerEffects_.erase(evadeIt);
                int counter = hitEnemy(i, mitigate(artPower(), enemyDefense(i)));
                Sfx::play(SfxId::Hit, sfxDelay_);
                sfxDelay_ += 0.3f;
                logMessage("Esquivas a " + enemy->getName() + " y contraatacas por " + ts(counter) + " daño!");
                if (!enemy->isAlive()) logMessage("  " + enemy->getName() + " derrotado!");
                continue;
            }

            int rawDmg = std::max(1, enemyAttack(i) - playerDefense());

            // Orco: 2 ataques por turno pero cada golpe es al 60%
            if (enemy->getType() == EnemyType::Orc) {
                rawDmg = std::max(1, rawDmg * 60 / 100);
            }

            // Check DefendingHeavy first (consumed on first hit)
            auto heavyIt = std::find_if(playerEffects_.begin(), playerEffects_.end(),
                [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::DefendingHeavy; });
            if (heavyIt != playerEffects_.end()) {
                int blockPct = heavyIt->magnitude;
                playerEffects_.erase(heavyIt);
                Sfx::play(SfxId::Blocked, sfxDelay_);
                // Escudo Arcano (100%) absorbe el golpe entero; Escudo Total (70%) lo reduce
                if (blockPct >= 100) {
                    logMessage("Escudo Arcano! Absorbe el golpe de " + enemy->getName() + ".");
                    sfxDelay_ += 0.3f;
                    continue;
                }
                rawDmg = std::max(1, rawDmg * (100 - blockPct) / 100);
                logMessage("Escudo Total! Daño reducido " + ts(blockPct) + "%.");
            } else {
                bool defending = std::any_of(playerEffects_.begin(), playerEffects_.end(),
                    [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Defending; });
                if (defending) {
                    rawDmg = std::max(1, rawDmg / 2);
                    logMessage("Postura defensiva! Daño reducido 50%.");
                    Sfx::play(SfxId::Blocked, sfxDelay_);
                }
            }

            logMessage(enemy->getName() + " ataca a " + player_.getName()
                       + " por " + ts(rawDmg) + " daño.");
            hurtPlayer(rawDmg);
            sfxDelay_ += 0.3f;

            // Araña: envenena al golpear
            if (enemy->getType() == EnemyType::Spider) {
                bool already = std::any_of(playerEffects_.begin(), playerEffects_.end(),
                    [](const StatusEffect& fx){ return fx.type == StatusEffect::Type::Poisoned; });
                if (!already) {
                    StatusEffect p; p.type = StatusEffect::Type::Poisoned; p.turnsLeft = 3; p.magnitude = 5;
                    playerEffects_.push_back(p);
                    logMessage("La Araña te envenena!");
                }
            }
            // Vampiro: se cura 30% del daño infligido
            if (enemy->getType() == EnemyType::Vampire) {
                int heal = std::max(1, rawDmg * 30 / 100);
                enemy->heal(heal);
                logMessage(enemy->getName() + " drena " + ts(heal) + " HP!");
            }

            if (!player_.isAlive()) {
                logMessage(player_.getName() + " ha caído en batalla...");
                phase_ = CombatPhase::CombatOver;
                return;
            }
        }
    }

    // Tick effects de duración después de los ataques (Defending, AttackBoosted)
    auto timed = [](StatusEffect::Type t) {
        using T = StatusEffect::Type;
        return t == T::AttackBoosted || t == T::Exposed || t == T::DefenseBoosted
            || t == T::Undying || t == T::Evading;
    };
    for (auto& fx : playerEffects_)
        if (timed(fx.type)) fx.turnsLeft--;
    playerEffects_.erase(
        std::remove_if(playerEffects_.begin(), playerEffects_.end(),
            [&](const StatusEffect& fx) {
                return fx.type == StatusEffect::Type::Defending
                    || (timed(fx.type) && fx.turnsLeft <= 0);
            }),
        playerEffects_.end());

    // Efectos de enemigo con duración: Congelado, Rompecorazas, Colapso, Maldición/Marca
    auto enemyTimed = [](StatusEffect::Type t) {
        using T = StatusEffect::Type;
        return t == T::Frozen || t == T::Sundered || t == T::Weakened || t == T::Vulnerable;
    };
    for (auto& fxv : enemyEffects_) {
        for (auto& fx : fxv)
            if (enemyTimed(fx.type)) fx.turnsLeft--;
        fxv.erase(
            std::remove_if(fxv.begin(), fxv.end(),
                [&](const StatusEffect& fx) { return enemyTimed(fx.type) && fx.turnsLeft <= 0; }),
            fxv.end());
    }

    // Restore player AP
    currentAp_ = maxAp_;
    logMessage("--- Turno del jugador  PA: " + ts(currentAp_) + "/" + ts(maxAp_) + " ---");
    phase_ = CombatPhase::PlayerTurn;
    checkCombatOver();
    sfxDelay_ = 0.0f;
}

void CombatSystem::tickPlayerEffects() {
    int poison = 0;
    for (auto& fx : playerEffects_) {
        if (fx.type == StatusEffect::Type::Poisoned) {
            poison += fx.magnitude;
            fx.turnsLeft--;
        }
        if (fx.type == StatusEffect::Type::Regen) {
            player_.heal(fx.magnitude);
            logMessage(player_.getName() + " regenera " + ts(fx.magnitude) + " HP.");
            fx.turnsLeft--;
        }
    }
    if (poison > 0) {
        logMessage(player_.getName() + " sufre " + ts(poison) + " daño por veneno.");
        hurtPlayer(poison);
    }
    playerEffects_.erase(
        std::remove_if(playerEffects_.begin(), playerEffects_.end(),
            [](const StatusEffect& fx) {
                return (fx.type == StatusEffect::Type::Poisoned ||
                        fx.type == StatusEffect::Type::Regen) && fx.turnsLeft <= 0;
            }),
        playerEffects_.end());
}

void CombatSystem::tickEnemyEffects() {
    for (int i = 0; i < static_cast<int>(enemies_.size()); i++) {
        if (!enemies_[i]->isAlive()) { enemyEffects_[i].clear(); continue; }

        for (auto& fx : enemyEffects_[i]) {
            if (fx.type == StatusEffect::Type::Poisoned) {
                enemies_[i]->takeDamageRaw(fx.magnitude);
                logMessage(enemies_[i]->getName() + " sufre " + ts(fx.magnitude)
                           + " daño por veneno.");
                if (!enemies_[i]->isAlive())
                    logMessage("  " + enemies_[i]->getName() + " muere envenenado!");
                fx.turnsLeft--;
            }
            if (fx.type == StatusEffect::Type::TrapPending) {
                enemies_[i]->takeDamageRaw(fx.magnitude);
                logMessage("Trampa! " + enemies_[i]->getName() + " sufre "
                           + ts(fx.magnitude) + " daño.");
                if (!enemies_[i]->isAlive())
                    logMessage("  " + enemies_[i]->getName() + " derrotado!");
                fx.turnsLeft--;
            }
        }

        enemyEffects_[i].erase(
            std::remove_if(enemyEffects_[i].begin(), enemyEffects_[i].end(),
                [](const StatusEffect& fx) {
                    return (fx.type == StatusEffect::Type::Poisoned
                         || fx.type == StatusEffect::Type::TrapPending)
                        && fx.turnsLeft <= 0;
                }),
            enemyEffects_[i].end());
    }
}

// ─── helpers de efectos ──────────────────────────────────────────────────────

int CombatSystem::enemyDefense(int idx) const {
    int def = enemies_[idx]->getDefense();
    for (const auto& fx : enemyEffects_[idx])
        if (fx.type == StatusEffect::Type::Sundered)
            def = def * (100 - fx.magnitude) / 100;
    return def;
}

int CombatSystem::enemyAttack(int idx) const {
    int atk = enemies_[idx]->getAttack();
    for (const auto& fx : enemyEffects_[idx])
        if (fx.type == StatusEffect::Type::Weakened)
            atk = atk * (100 - fx.magnitude) / 100;
    return std::max(1, atk);
}

int CombatSystem::playerDefense() const {
    int def = player_.getDefense();
    for (const auto& fx : playerEffects_) {
        if (fx.type == StatusEffect::Type::Exposed)
            def = def * (100 - fx.magnitude) / 100;
        if (fx.type == StatusEffect::Type::DefenseBoosted)
            def += fx.magnitude;
    }
    return def;
}

int CombatSystem::hitEnemy(int idx, int dmg) {
    int bonus = 0;
    for (const auto& fx : enemyEffects_[idx])
        if (fx.type == StatusEffect::Type::Vulnerable)
            bonus = std::max(bonus, fx.magnitude);
    dmg = std::max(1, dmg * (100 + bonus) / 100);
    bool wasAlive = enemies_[idx]->isAlive();
    enemies_[idx]->takeDamageRaw(dmg);
    if (wasAlive && !enemies_[idx]->isAlive())
        Sfx::play(enemies_[idx]->isBoss() ? SfxId::BossDie : SfxId::EnemyDie, sfxDelay_ + 0.1f);
    return dmg;
}

void CombatSystem::hurtPlayer(int dmg) {
    if (playerHas(StatusEffect::Type::Undying) && player_.getHp() - dmg < 1) {
        dmg = std::max(0, player_.getHp() - 1);
        logMessage("Último Bastión! Resistes en pie.");
    }
    player_.takeDamageRaw(dmg);
    Sfx::play(SfxId::PlayerHurt, sfxDelay_);
    if (player_.isAlive()) return;
    auto it = std::find_if(playerEffects_.begin(), playerEffects_.end(),
        [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Revive; });
    if (it == playerEffects_.end()) {
        Sfx::play(SfxId::Defeat, sfxDelay_ + 0.3f);
        return;
    }
    Sfx::play(SfxId::Heal, sfxDelay_ + 0.3f);
    int heal = std::max(1, player_.getMaxHp() * it->magnitude / 100);
    playerEffects_.erase(it);
    player_.heal(heal);
    logMessage("Génesis! Renaces con " + ts(heal) + " HP.");
}

void CombatSystem::addEnemyEffect(int idx, StatusEffect::Type type, int turns, int magnitude) {
    // Reaplicar el mismo efecto renueva su duración en vez de acumularlo
    for (auto& fx : enemyEffects_[idx]) {
        if (fx.type == type) {
            fx.turnsLeft = std::max(fx.turnsLeft, turns);
            fx.magnitude = std::max(fx.magnitude, magnitude);
            return;
        }
    }
    enemyEffects_[idx].push_back({ type, turns, magnitude });
}

void CombatSystem::addPlayerEffect(StatusEffect::Type type, int turns, int magnitude) {
    for (auto& fx : playerEffects_) {
        if (fx.type == type) {
            fx.turnsLeft = std::max(fx.turnsLeft, turns);
            fx.magnitude = std::max(fx.magnitude, magnitude);
            return;
        }
    }
    playerEffects_.push_back({ type, turns, magnitude });
}

bool CombatSystem::playerHas(StatusEffect::Type type) const {
    return std::any_of(playerEffects_.begin(), playerEffects_.end(),
        [type](const StatusEffect& fx) { return fx.type == type; });
}

// ─── resolveArt ──────────────────────────────────────────────────────────────

bool CombatSystem::resolveArt(ArtEffect effect) {
    using T = StatusEffect::Type;
    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    const int ti = currentTarget_;
    auto& target = enemies_[ti];
    const int n  = static_cast<int>(enemies_.size());

    // Los avisos de "derrotado" se dan al final, después del mensaje del arte
    std::vector<bool> wasAlive(n);
    for (int i = 0; i < n; i++) wasAlive[i] = enemies_[i]->isAlive();

    auto selfOnly = [](ArtEffect e) {
        switch (e) {
            case ArtEffect::GritoDeGuerra: case ArtEffect::EscudoTotal:
            case ArtEffect::Creacion:      case ArtEffect::PresenciaDivina:
            case ArtEffect::BerserkWarrior: case ArtEffect::Fortaleza:
            case ArtEffect::EscudoArcano:  case ArtEffect::UltimoBastion:
            case ArtEffect::Sombra:        case ArtEffect::Bendicion:
            case ArtEffect::Genesis:       case ArtEffect::MundoNuevo:
            case ArtEffect::Paradoja:
                return true;
            default:
                return false;
        }
    };
    if (!selfOnly(effect) && !target->isAlive()) {
        logMessage("No hay objetivos vivos.");
        return false;
    }
    artSelf_ = selfOnly(effect);

    const int power = artPower();
    auto single = [&](int dmg) { return hitEnemy(ti, dmg); };
    auto allEnemies = [&](auto dmgFor) {
        for (int i = 0; i < n; i++)
            if (enemies_[i]->isAlive()) hitEnemy(i, dmgFor(i));
    };
    // Pérdida de PA: los jefes solo pierden 1 para que no queden sin turno siempre
    auto stun = [&](int idx, int pa, int turns) {
        int lost = enemies_[idx]->isBoss() ? 1 : pa;
        addEnemyEffect(idx, T::Frozen, turns, lost);
        return lost;
    };

    switch (effect) {

        // ─── Guerrero ────────────────────────────────────────────────────────
        case ArtEffect::GolpeDemoledor: {
            int raw = ranked(static_cast<int>(getEffectiveAttack() * artLevelScale(player_.getLevel()) * 1.4f));
            int dmg = single(mitigate(raw, enemyDefense(ti) / 2));
            logMessage("Golpe Demoledor! " + ts(dmg) + " daño (ignora mitad DEF).");
            break;
        }
        case ArtEffect::GritoDeGuerra: {
            int mag = ranked(player_.getAttack() / 3 + 2);
            addPlayerEffect(T::AttackBoosted, 2, mag);
            logMessage("Grito de Guerra! ATK +" + ts(mag) + " por 2 turnos.");
            break;
        }
        case ArtEffect::EscudoTotal: {
            addPlayerEffect(T::DefendingHeavy, 1, 70);
            logMessage("Escudo Total! -70% daño en el próximo golpe.");
            break;
        }
        case ArtEffect::BerserkWarrior: {
            int mag = ranked(player_.getAttack() / 2);
            addPlayerEffect(T::AttackBoosted, 3, mag);
            addPlayerEffect(T::Exposed, 3, 30);
            logMessage("Furia Berserker! ATK +" + ts(mag) + " y DEF -30% por 3 turnos.");
            break;
        }
        case ArtEffect::Fortaleza: {
            if (playerHas(T::Regen)) { logMessage("Ya te estás regenerando."); return false; }
            int mag = std::max(1, ranked(player_.getMaxHp() / 10));
            addPlayerEffect(T::Regen, 3, mag);
            logMessage("Fortaleza! Regeneras " + ts(mag) + " HP por turno durante 3 turnos.");
            break;
        }
        case ArtEffect::Terremoto: {
            int raw = static_cast<int>(power * 1.2f);
            logMessage("Terremoto! La tierra golpea a todos.");
            allEnemies([&](int i) { return mitigate(raw, enemyDefense(i)); });
            break;
        }
        case ArtEffect::Ejecutar: {
            bool low = target->getHp() * 100 < target->getMaxHp() * 30;
            int raw  = static_cast<int>(power * 1.5f) * (low ? 3 : 1);
            int dmg  = single(mitigate(raw, enemyDefense(ti)));
            logMessage(std::string(low ? "EJECUCIÓN! " : "Ejecutar: ") + ts(dmg) + " daño.");
            break;
        }
        case ArtEffect::Rompecorazas: {
            int dmg = single(mitigate(power, enemyDefense(ti)));
            addEnemyEffect(ti, T::Sundered, 3, std::min(90, ranked(50)));
            logMessage("Rompecorazas! " + ts(dmg) + " daño y " + target->getName()
                       + " pierde la mitad de su DEF por 3 turnos.");
            break;
        }
        case ArtEffect::UltimoBastion: {
            addPlayerEffect(T::Undying, 2, 1);
            logMessage("Último Bastión! Tu vida no bajará de 1 durante 2 turnos.");
            break;
        }

        // ─── Mago ────────────────────────────────────────────────────────────
        case ArtEffect::BolaDeFuego: {
            int dmg = power + 10;
            logMessage("Bola de Fuego! " + ts(dmg) + " daño mágico a todos!");
            allEnemies([&](int) { return dmg; });
            break;
        }
        case ArtEffect::Congelar: {
            stun(ti, 1, 1);
            logMessage(target->getName() + " congelado! -1 PA en su turno.");
            break;
        }
        case ArtEffect::Drenaje: {
            int dmg = single(mitigate(power + 5, enemyDefense(ti)));
            player_.heal(dmg);
            logMessage("Drenaje! " + ts(dmg) + " daño, recuperas " + ts(dmg) + " HP.");
            break;
        }
        case ArtEffect::Rayo: {
            int dmg = single(std::max(1, static_cast<int>(power * 1.8f)));
            logMessage("Rayo! " + ts(dmg) + " daño devastador (ignora DEF).");
            break;
        }
        case ArtEffect::TormentaArcana: {
            int dmg = std::max(1, power / 2);
            logMessage("Tormenta Arcana! " + ts(dmg) + " daño a todos y congelados!");
            for (int i = 0; i < n; i++) {
                if (!enemies_[i]->isAlive()) continue;
                hitEnemy(i, dmg);
                stun(i, 1, 1);
            }
            break;
        }
        case ArtEffect::EscudoArcano: {
            addPlayerEffect(T::DefendingHeavy, 1, 100);
            logMessage("Escudo Arcano! El próximo golpe será absorbido.");
            break;
        }
        case ArtEffect::CadenaRayos: {
            int dmg = std::max(1, static_cast<int>(power * 1.4f));
            int first = single(dmg);
            logMessage("Cadena de Rayos! " + ts(first) + " a " + target->getName() + ".");
            int jumps = 0;
            for (int i = 0; i < n && jumps < 2; i++) {
                if (i == ti || !enemies_[i]->isAlive()) continue;
                dmg = std::max(1, dmg * 7 / 10);
                int d = hitEnemy(i, dmg);
                logMessage("  Salta a " + enemies_[i]->getName() + ": " + ts(d) + " daño.");
                jumps++;
            }
            break;
        }
        case ArtEffect::Meteoro: {
            int raw = static_cast<int>(power * 2.2f);
            logMessage("Meteoro! El cielo cae sobre todos.");
            allEnemies([&](int i) { return mitigate(raw, enemyDefense(i)); });
            break;
        }
        case ArtEffect::Maldicion: {
            int pct = ranked(30);
            addEnemyEffect(ti, T::Vulnerable, 3, pct);
            logMessage(target->getName() + " maldito! Recibe +" + ts(pct) + "% de daño por 3 turnos.");
            break;
        }

        // ─── Arquera ─────────────────────────────────────────────────────────
        case ArtEffect::DisparoDoble: {
            for (int shot = 1; shot <= 2 && target->isAlive(); shot++) {
                int dmg = single(mitigate(power, enemyDefense(ti)));
                logMessage("Disparo " + ts(shot) + ": " + ts(dmg) + " daño.");
            }
            break;
        }
        case ArtEffect::Trampa: {
            bool already = std::any_of(enemyEffects_[ti].begin(), enemyEffects_[ti].end(),
                [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::TrapPending; });
            if (already) { logMessage(target->getName() + " ya tiene una trampa activa."); return false; }
            enemyEffects_[ti].push_back({ T::TrapPending, 1, power });
            logMessage("Trampa colocada! " + target->getName() + " recibirá " + ts(power) + " daño.");
            break;
        }
        case ArtEffect::Veneno: {
            bool already = std::any_of(enemyEffects_[ti].begin(), enemyEffects_[ti].end(),
                [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Poisoned; });
            if (already) { logMessage(target->getName() + " ya está envenenado."); return false; }
            int mag = std::max(1, power / 3);
            enemyEffects_[ti].push_back({ T::Poisoned, 3, mag });
            logMessage(target->getName() + " envenenado! " + ts(mag) + " daño/turno por 3 turnos.");
            break;
        }
        case ArtEffect::FlechaPerforante: {
            int dmg = single(std::max(1, static_cast<int>(power * 1.6f)));
            logMessage("Flecha Perforante! " + ts(dmg) + " daño (ignora DEF).");
            break;
        }
        case ArtEffect::GolpeCritico: {
            int dmg = single(mitigate(static_cast<int>(power * 2.5f), enemyDefense(ti)));
            logMessage("Tiro Certero! " + ts(dmg) + " daño crítico.");
            break;
        }
        case ArtEffect::Red: {
            int lost = stun(ti, 1, 2);
            logMessage(target->getName() + " atrapado en la red! -" + ts(lost) + " PA durante 2 turnos.");
            break;
        }
        case ArtEffect::LluviaFlechas: {
            logMessage("Lluvia de Flechas! 3 impactos aleatorios!");
            for (int h = 0; h < 3; h++) {
                std::vector<int> alive;
                for (int i = 0; i < n; i++)
                    if (enemies_[i]->isAlive()) alive.push_back(i);
                if (alive.empty()) break;
                int idx = alive[std::rand() % alive.size()];
                int dmg = hitEnemy(idx, mitigate(static_cast<int>(power * 0.6f), enemyDefense(idx)));
                logMessage("  Flecha " + ts(h + 1) + ": " + ts(dmg) + " a " + enemies_[idx]->getName());
            }
            break;
        }
        case ArtEffect::MarcaCazador: {
            int pct = ranked(50);
            addEnemyEffect(ti, T::Vulnerable, 3, pct);
            logMessage(target->getName() + " marcado! Recibe +" + ts(pct) + "% de daño por 3 turnos.");
            break;
        }
        case ArtEffect::Sombra: {
            addPlayerEffect(T::Evading, 1, 1);
            logMessage("Paso Sombrío! Esquivarás el próximo golpe y contraatacarás.");
            break;
        }

        // ─── Halley ──────────────────────────────────────────────────────────
        case ArtEffect::DistorsionTemporal: {
            int lost = stun(ti, 10, 1);
            logMessage("Distorsión Temporal! " + target->getName()
                       + (lost == 1 ? " pierde 1 PA (resiste)." : " pierde todos sus PA."));
            break;
        }
        case ArtEffect::VinculoEterno: {
            int dmg = single(mitigate(power, enemyDefense(ti)));
            int heal = dmg * 3 / 2;
            player_.heal(heal);
            logMessage("Vínculo Eterno! " + ts(dmg) + " daño, recuperas " + ts(heal) + " HP.");
            break;
        }
        case ArtEffect::JuicioFinal: {
            int dmg = single(std::max(1, power * 2));
            logMessage("Juicio Final! " + ts(dmg) + " daño imparable.");
            break;
        }
        case ArtEffect::AbsorbVida: {
            int pct    = ranked(target->isBoss() ? 10 : 30);
            int stolen = single(std::max(1, target->getHp() * pct / 100));
            player_.heal(stolen);
            logMessage("Absorber Vida! Robas " + ts(stolen) + " HP a " + target->getName() + ".");
            break;
        }
        case ArtEffect::CampoEquilibrio: {
            int avg  = (player_.getHp() + target->getHp()) / 2;
            int diff = avg - player_.getHp();
            int enemyDmg = std::max(0, target->getHp() - avg);
            // Los jefes resisten: como mucho pierden el 15% de su vida actual
            // y la curación no supera lo que se les quita
            if (target->isBoss()) {
                enemyDmg = std::min(enemyDmg, target->getHp() * 15 / 100);
                if (diff > 0) diff = std::min(diff, enemyDmg);
            }
            if (diff > 0) player_.heal(diff);
            else          hurtPlayer(-diff);
            if (enemyDmg > 0) target->takeDamageRaw(enemyDmg);
            if (target->isBoss())
                logMessage("Campo de Equilibrio! " + target->getName() + " resiste: pierde " +
                           ts(enemyDmg) + " HP.");
            else
                logMessage("Campo de Equilibrio! Vuestras vidas se igualan en " + ts(avg) + ".");
            break;
        }
        case ArtEffect::ColapsoDimensional: {
            int dmg = single(std::max(1, static_cast<int>(power * 1.5f)));
            int pct = ranked(30);
            addEnemyEffect(ti, T::Sundered, 3, pct);
            addEnemyEffect(ti, T::Weakened, 3, pct);
            logMessage("Colapso Dimensional! " + ts(dmg) + " daño; " + target->getName()
                       + " pierde " + ts(pct) + "% de ATK y DEF por 3 turnos.");
            break;
        }
        case ArtEffect::DetenerTiempo: {
            logMessage("Detener el Tiempo! Los enemigos se ralentizan.");
            for (int i = 0; i < n; i++)
                if (enemies_[i]->isAlive()) stun(i, 2, 1);
            break;
        }
        case ArtEffect::Paradoja: {
            if (!hasLastArt_) { logMessage("Paradoja: no hay habilidad que repetir."); return false; }
            logMessage("Paradoja! El tiempo se repite...");
            return resolveArt(lastArt_);
        }
        case ArtEffect::Sentencia: {
            int missing = target->getMaxHp() - target->getHp();
            int pct     = ranked(target->isBoss() ? 20 : 50);
            int dmg     = single(std::max(1, missing * pct / 100 + power));
            logMessage("Sentencia! " + ts(dmg) + " daño (castiga las heridas).");
            break;
        }

        // ─── Nato ────────────────────────────────────────────────────────────
        case ArtEffect::LluviaEstelar: {
            int dmg = std::max(1, static_cast<int>(power * 1.5f));
            logMessage("Juicio Estelar! " + ts(dmg) + " daño divino a todos!");
            allEnemies([&](int) { return dmg; });
            break;
        }
        case ArtEffect::Creacion: {
            int heal = ranked(player_.getMaxHp() / 2);
            player_.heal(heal);
            logMessage("Creación! Recuperas " + ts(heal) + " HP.");
            break;
        }
        case ArtEffect::PresenciaDivina: {
            int mag = ranked(player_.getAttack() / 2 + 5);
            addPlayerEffect(T::AttackBoosted, 3, mag);
            logMessage("Presencia Divina! ATK +" + ts(mag) + " por 3 turnos.");
            break;
        }
        case ArtEffect::LanzaLuz: {
            int dmg = single(mitigate(power * 2, enemyDefense(ti) / 2));
            logMessage("Lanza de Luz! " + ts(dmg) + " daño (ignora mitad DEF).");
            break;
        }
        case ArtEffect::Bendicion: {
            int regen = std::max(1, ranked(player_.getMaxHp() * 8 / 100));
            int def   = ranked(player_.getDefense() / 2 + 3);
            addPlayerEffect(T::Regen, 3, regen);
            addPlayerEffect(T::DefenseBoosted, 3, def);
            logMessage("Bendición! +" + ts(regen) + " HP por turno y DEF +" + ts(def) + " por 3 turnos.");
            break;
        }
        case ArtEffect::Exilio: {
            int lost = stun(ti, 10, 2);
            logMessage("Exilio! " + target->getName()
                       + (lost == 1 ? " resiste: -1 PA durante 2 turnos." : " pierde sus turnos durante 2 turnos."));
            break;
        }
        case ArtEffect::Supernova: {
            int dmg = std::max(1, power * 2);
            logMessage("Supernova! " + ts(dmg) + " daño a todos (ignora DEF).");
            allEnemies([&](int) { return dmg; });
            break;
        }
        case ArtEffect::Genesis: {
            if (playerHas(T::Revive)) { logMessage("Génesis ya te protege."); return false; }
            addPlayerEffect(T::Revive, 1, std::min(100, ranked(50)));
            logMessage("Génesis! Si caes en este combate, renacerás.");
            break;
        }
        case ArtEffect::MundoNuevo: {
            int heal = ranked(player_.getMaxHp() / 4);
            player_.heal(heal);
            playerEffects_.erase(
                std::remove_if(playerEffects_.begin(), playerEffects_.end(),
                    [](const StatusEffect& fx) {
                        return fx.type == StatusEffect::Type::Poisoned
                            || fx.type == StatusEffect::Type::Exposed;
                    }),
                playerEffects_.end());
            currentAp_ += 2;
            logMessage("Mundo Nuevo! +" + ts(heal) + " HP, efectos negativos eliminados y +2 PA.");
            break;
        }
    }

    for (int i = 0; i < n; i++)
        if (wasAlive[i] && !enemies_[i]->isAlive())
            logMessage("  " + enemies_[i]->getName() + " derrotado!");
    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    return true;
}

int CombatSystem::mitigate(int raw, int def) {
    return std::max({ 1, raw * kMinDamagePct / 100, raw - def });
}

float CombatSystem::artLevelScale(int level) {
    return 1.0f + kArtLevelScalePct * (level - 1) / 100.0f;
}

int CombatSystem::artPower() const {
    return ranked(static_cast<int>(player_.getAttack() * artLevelScale(player_.getLevel())));
}

int CombatSystem::ranked(int value) const {
    return value * (100 + artRankBonusPct(artRank_)) / 100;
}

bool CombatSystem::rollCritical() const {
    int chance = 15;
    switch (player_.getClass()) {
        case PlayerClass::Ranger:     chance = 20; break;
        case PlayerClass::Halley:     chance = 18; break;
        case PlayerClass::Nato:       chance = 20; break;
        default: break;
    }
    return (std::rand() % 100) < chance;
}

bool CombatSystem::isPlayerAttackBoosted() const {
    return std::any_of(playerEffects_.begin(), playerEffects_.end(),
        [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::AttackBoosted; });
}

int CombatSystem::getPlayerAttackBoost() const {
    int total = 0;
    for (const auto& fx : playerEffects_)
        if (fx.type == StatusEffect::Type::AttackBoosted)
            total += fx.magnitude;
    return total;
}

int CombatSystem::getEffectiveAttack() const {
    int atk = player_.getAttack();
    if (player_.getClass() == PlayerClass::Warrior) {
        int pct = player_.getMana() * 100 / std::max(1, player_.getMaxMana());
        int factor = std::max(70, pct);
        atk = atk * factor / 100;
    }
    return atk;
}
