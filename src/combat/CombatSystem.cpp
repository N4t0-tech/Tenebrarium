#include "CombatSystem.hpp"
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
      currentAp_(maxAp_),
      phase_(CombatPhase::PlayerTurn),
      fled_(false)
{
    enemyEffects_.resize(enemies_.size());
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

void CombatSystem::doAttack() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes para atacar."); return; }

    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    auto& target = enemies_[currentTarget_];
    if (!target->isAlive()) { logMessage("No hay objetivos vivos."); return; }

    int rawAtk = player_.getAttack();
    if (isPlayerAttackBoosted()) rawAtk += getPlayerAttackBoost();
    int dmg = std::max(1, rawAtk - target->getDefense());
    target->takeDamageRaw(dmg);

    logMessage(player_.getName() + " ataca a " + target->getName()
               + " por " + ts(dmg) + " daño.");
    if (!target->isAlive()) {
        logMessage("  " + target->getName() + " derrotado!");
        advanceTarget();
    }

    currentAp_--;
    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doHeavyAttack() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(2)) { logMessage("PA insuficientes para ataque fuerte (necesitas 2)."); return; }

    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    auto& target = enemies_[currentTarget_];
    if (!target->isAlive()) { logMessage("No hay objetivos vivos."); return; }

    int rawAtk = static_cast<int>(player_.getAttack() * 1.8f);
    if (isPlayerAttackBoosted()) rawAtk += getPlayerAttackBoost();
    int dmg = std::max(1, rawAtk - target->getDefense());
    target->takeDamageRaw(dmg);

    logMessage(player_.getName() + " Ataque Fuerte a " + target->getName()
               + " por " + ts(dmg) + " daño!");
    if (!target->isAlive()) {
        logMessage("  " + target->getName() + " derrotado!");
        advanceTarget();
    }

    currentAp_ -= 2;
    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
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
    resolveArt(art.effect);

    currentAp_ -= art.apCost;
    checkCombatOver();
    if (isOver()) return;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doDefend() {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes para defender."); return; }

    StatusEffect fx;
    fx.type      = StatusEffect::Type::Defending;
    fx.turnsLeft = 1;
    fx.magnitude = 50;
    playerEffects_.push_back(fx);
    logMessage(player_.getName() + " adopta postura defensiva (-50% daño).");

    currentAp_--;
    if (currentAp_ <= 0) { logMessage("Sin PA. Turno enemigo..."); processEnemyTurn(); }
}

void CombatSystem::doUseItem(int /*i*/) {
    if (phase_ != CombatPhase::PlayerTurn) return;
    if (!hasEnoughAp(1)) { logMessage("PA insuficientes."); return; }
    int healed = player_.useConsumable();
    if (healed == 0) {
        logMessage("No tienes pociones.");
        return;
    }
    logMessage(player_.getName() + " usa una pocion y recupera " +
               std::to_string(healed) + " HP.");
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
    if (!hasEnoughAp(3)) {
        logMessage("PA insuficientes para huir (necesitas 3 PA).");
        return;
    }

    currentAp_ -= 3;
    if ((std::rand() % 100) < 70) {
        fled_ = true;
        logMessage(player_.getName() + " huye del combate!");
        phase_ = CombatPhase::CombatOver;
    } else {
        logMessage("!" + player_.getName() + " intenta huir pero falla!");
        if (currentAp_ <= 0) processEnemyTurn();
    }
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

void CombatSystem::checkCombatOver() {
    if (!player_.isAlive() || fled_) {
        phase_ = CombatPhase::CombatOver;
        return;
    }
    bool allDead = std::all_of(enemies_.begin(), enemies_.end(),
        [](const std::unique_ptr<Enemy>& e) { return !e->isAlive(); });
    if (allDead) {
        phase_ = CombatPhase::CombatOver;
        logMessage("=== Victoria! Todos los enemigos derrotados. ===");
    }
}

void CombatSystem::processEnemyTurn() {
    if (phase_ == CombatPhase::CombatOver) return;
    phase_ = CombatPhase::EnemyTurn;
    logMessage("--- Turno enemigo ---");

    tickPlayerEffects();
    tickEnemyEffects();

    checkCombatOver();
    if (isOver()) return;

    for (int i = 0; i < static_cast<int>(enemies_.size()); i++) {
        auto& enemy = enemies_[i];
        if (!enemy->isAlive()) continue;

        // Check frozen: reduce PA
        int attackCount = enemy->getBasePa();
        bool frozen = std::any_of(enemyEffects_[i].begin(), enemyEffects_[i].end(),
            [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Frozen; });
        if (frozen) {
            attackCount = std::max(0, attackCount - 1);
            logMessage(enemy->getName() + " esta congelado (-1 PA).");
        }

        for (int pa = 0; pa < attackCount; pa++) {
            if (!player_.isAlive()) break;

            int rawDmg = std::max(1, enemy->getAttack() - player_.getDefense());

            // Check DefendingHeavy first (consumed on first hit)
            auto heavyIt = std::find_if(playerEffects_.begin(), playerEffects_.end(),
                [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::DefendingHeavy; });
            if (heavyIt != playerEffects_.end()) {
                rawDmg = std::max(1, rawDmg * 30 / 100);
                playerEffects_.erase(heavyIt);
                logMessage("Escudo Total! Dano reducido 70%.");
            } else {
                bool defending = std::any_of(playerEffects_.begin(), playerEffects_.end(),
                    [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::Defending; });
                if (defending) {
                    rawDmg = std::max(1, rawDmg / 2);
                    logMessage("Postura defensiva! Dano reducido 50%.");
                }
            }

            player_.takeDamageRaw(rawDmg);
            logMessage(enemy->getName() + " ataca a " + player_.getName()
                       + " por " + ts(rawDmg) + " daño.");

            if (!player_.isAlive()) {
                logMessage(player_.getName() + " ha caido en batalla...");
                phase_ = CombatPhase::CombatOver;
                return;
            }
        }
    }

    // Restore player AP
    currentAp_ = maxAp_;
    logMessage("--- Turno del jugador  PA: " + ts(currentAp_) + "/" + ts(maxAp_) + " ---");
    phase_ = CombatPhase::PlayerTurn;
    checkCombatOver();
}

void CombatSystem::tickPlayerEffects() {
    for (auto& fx : playerEffects_) fx.turnsLeft--;
    playerEffects_.erase(
        std::remove_if(playerEffects_.begin(), playerEffects_.end(),
            [](const StatusEffect& fx) { return fx.turnsLeft <= 0; }),
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
            }
            if (fx.type == StatusEffect::Type::TrapPending) {
                enemies_[i]->takeDamageRaw(fx.magnitude);
                logMessage("Trampa! " + enemies_[i]->getName() + " sufre "
                           + ts(fx.magnitude) + " daño.");
                if (!enemies_[i]->isAlive())
                    logMessage("  " + enemies_[i]->getName() + " derrotado!");
            }
            fx.turnsLeft--;
        }

        enemyEffects_[i].erase(
            std::remove_if(enemyEffects_[i].begin(), enemyEffects_[i].end(),
                [](const StatusEffect& fx) { return fx.turnsLeft <= 0; }),
            enemyEffects_[i].end());
    }
}

void CombatSystem::resolveArt(ArtEffect effect) {
    if (!enemies_[currentTarget_]->isAlive()) advanceTarget();
    auto& target = enemies_[currentTarget_];

    switch (effect) {

        case ArtEffect::GolpeDemoledor: {
            if (!target->isAlive()) break;
            int dmg = std::max(1, player_.getAttack());  // ignores DEF
            target->takeDamageRaw(dmg);
            logMessage("Golpe Demoledor! " + ts(dmg) + " daño (ignora DEF).");
            if (!target->isAlive()) {
                logMessage("  " + target->getName() + " derrotado!");
                advanceTarget();
            }
            break;
        }

        case ArtEffect::GritoDeGuerra: {
            StatusEffect fx;
            fx.type      = StatusEffect::Type::AttackBoosted;
            fx.turnsLeft = 2;
            fx.magnitude = player_.getAttack() / 3 + 2;  // ~30% ATK bonus
            playerEffects_.push_back(fx);
            logMessage("Grito de Guerra! ATK +" + ts(fx.magnitude) + " por 2 turnos.");
            break;
        }

        case ArtEffect::EscudoTotal: {
            StatusEffect fx;
            fx.type      = StatusEffect::Type::DefendingHeavy;
            fx.turnsLeft = 1;
            fx.magnitude = 70;
            playerEffects_.push_back(fx);
            logMessage("Escudo Total! -70% daño en el proximo golpe.");
            break;
        }

        case ArtEffect::BolaDeFuego: {
            int dmg = player_.getAttack() + 10;
            logMessage("Bola de Fuego! " + ts(dmg) + " daño magico a todos!");
            for (auto& e : enemies_) {
                if (e->isAlive()) {
                    e->takeDamageRaw(dmg);
                    if (!e->isAlive())
                        logMessage("  " + e->getName() + " derrotado!");
                }
            }
            advanceTarget();
            break;
        }

        case ArtEffect::Congelar: {
            if (!target->isAlive()) break;
            StatusEffect fx;
            fx.type      = StatusEffect::Type::Frozen;
            fx.turnsLeft = 1;
            fx.magnitude = 1;
            enemyEffects_[currentTarget_].push_back(fx);
            logMessage(target->getName() + " congelado! -1 PA en su turno.");
            break;
        }

        case ArtEffect::Drenaje: {
            if (!target->isAlive()) break;
            int dmg = std::max(1, player_.getAttack() - target->getDefense() + 5);
            target->takeDamageRaw(dmg);
            player_.heal(dmg);
            logMessage("Drenaje! " + ts(dmg) + " daño, recuperas " + ts(dmg) + " HP.");
            if (!target->isAlive()) {
                logMessage("  " + target->getName() + " derrotado!");
                advanceTarget();
            }
            break;
        }

        case ArtEffect::DisparoDoble: {
            if (!target->isAlive()) break;
            for (int shot = 1; shot <= 2; shot++) {
                if (!target->isAlive()) break;
                int dmg = std::max(1, player_.getAttack() - target->getDefense());
                target->takeDamageRaw(dmg);
                logMessage("Disparo " + ts(shot) + ": " + ts(dmg) + " daño.");
            }
            if (!target->isAlive()) {
                logMessage("  " + target->getName() + " derrotado!");
                advanceTarget();
            }
            break;
        }

        case ArtEffect::Trampa: {
            if (!target->isAlive()) break;
            StatusEffect fx;
            fx.type      = StatusEffect::Type::TrapPending;
            fx.turnsLeft = 1;
            fx.magnitude = player_.getAttack();
            enemyEffects_[currentTarget_].push_back(fx);
            logMessage("Trampa colocada! " + target->getName()
                       + " recibira " + ts(fx.magnitude) + " daño.");
            break;
        }

        case ArtEffect::Veneno: {
            if (!target->isAlive()) break;
            StatusEffect fx;
            fx.type      = StatusEffect::Type::Poisoned;
            fx.turnsLeft = 3;
            fx.magnitude = std::max(1, player_.getAttack() / 3);
            enemyEffects_[currentTarget_].push_back(fx);
            logMessage(target->getName() + " envenenado! "
                       + ts(fx.magnitude) + " daño/turno por 3 turnos.");
            break;
        }
    }
}

bool CombatSystem::isPlayerAttackBoosted() const {
    return std::any_of(playerEffects_.begin(), playerEffects_.end(),
        [](const StatusEffect& fx) { return fx.type == StatusEffect::Type::AttackBoosted; });
}

int CombatSystem::getPlayerAttackBoost() const {
    for (const auto& fx : playerEffects_)
        if (fx.type == StatusEffect::Type::AttackBoosted)
            return fx.magnitude;
    return 0;
}
