#include "EnemyAI.hpp"
#include "../core/Game.hpp"
#include <thread>
#include <chrono>

void EnemyAI::run(Game& g)
{
    while (g.aiRunning_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        if (!g.aiRunning_) break;

        GameState s = g.state_.load(std::memory_order_acquire);
        if ((s != GameState::Exploration && s != GameState::Shop))
            continue;

        auto dungeon = g.dungeon_;
        if (!dungeon)
            continue;

        int idx = dungeon->aiTick();
        if (idx >= 0)
            g.pendingCombatEnemy_.store(idx, std::memory_order_release);

        g.pendingRedraw_.store(true, std::memory_order_release);
    }
}
