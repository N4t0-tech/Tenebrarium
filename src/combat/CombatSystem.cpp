#include "CombatSystem.hpp"
#include <algorithm>

CombatSystem::CombatSystem(Entity& player, std::vector<Entity*> enemies)
    : player_(player), enemies_(std::move(enemies)), currentEnemy_(0)
{}

bool CombatSystem::isOver() const {
    if (!player_.isAlive()) return true;
    return std::all_of(enemies_.begin(), enemies_.end(),
        [](const Entity* e) { return !e->isAlive(); });
}

bool CombatSystem::playerWon() const {
    return player_.isAlive() && std::all_of(enemies_.begin(), enemies_.end(),
        [](const Entity* e) { return !e->isAlive(); });
}

void CombatSystem::playerTurn(Action action, int /*actionIndex*/) {
    if (isOver()) return;

    switch (action) {
        case Action::Attack: {
            if (currentEnemy_ >= static_cast<int>(enemies_.size())) break;
            Entity* target = enemies_[currentEnemy_];
            target->takeDamage(player_.getAttack());
            logMessage(player_.getName() + " attacks " + target->getName() + "!");
            if (!target->isAlive()) {
                logMessage(target->getName() + " was defeated!");
            }
            break;
        }
        case Action::UseArt:
            // TODO: arts/abilities system
            logMessage("Arts not yet implemented.");
            break;
        case Action::UseItem:
            // TODO: use item from inventory
            logMessage("Use item not yet implemented.");
            break;
        case Action::Flee:
            logMessage(player_.getName() + " fled from battle!");
            // Signal flee by killing all enemies for now
            for (auto* e : enemies_) e->takeDamage(9999);
            return;
    }

    cleanDeadEnemies();
    if (!isOver()) enemyTurn();
}

void CombatSystem::enemyTurn() {
    for (auto* enemy : enemies_) {
        if (!enemy->isAlive()) continue;
        enemy->takeDamage(0); // placeholder: enemies will have their own attack logic
        player_.takeDamage(enemy->getAttack());
        logMessage(enemy->getName() + " attacks " + player_.getName() + "!");
    }
}

void CombatSystem::logMessage(const std::string& msg) {
    log_.push_back(msg);
}

void CombatSystem::cleanDeadEnemies() {
    // Keep dead enemies in the list so the log makes sense; just skip them in enemyTurn.
    if (currentEnemy_ < static_cast<int>(enemies_.size()) && !enemies_[currentEnemy_]->isAlive()) {
        currentEnemy_++;
    }
}
