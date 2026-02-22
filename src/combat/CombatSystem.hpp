#pragma once

#include "entities/Entity.hpp"
#include <vector>
#include <string>

// Manages a single combat encounter: turn order, action resolution, log.
class CombatSystem {
public:
    CombatSystem(Entity& player, std::vector<Entity*> enemies);

    enum class Action { Attack, UseArt, UseItem, Flee };

    // Returns false when combat is over.
    bool isOver() const;

    // Execute a player action. Enemies respond automatically.
    void playerTurn(Action action, int actionIndex = 0);

    const std::vector<std::string>& getLog() const { return log_; }
    bool playerWon() const;

private:
    Entity& player_;
    std::vector<Entity*> enemies_;
    std::vector<std::string> log_;
    int currentEnemy_;   // index of the enemy being targeted

    void enemyTurn();
    void logMessage(const std::string& msg);
    void cleanDeadEnemies();
};
