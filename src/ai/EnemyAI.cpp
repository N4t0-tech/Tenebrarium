#include "EnemyAI.hpp"
#include "Pathfinding.hpp"
#include "../core/Game.hpp"
#include <thread>
#include <chrono>

void EnemyAI::run(Game& g)
{
    while (g.aiRunning_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        if (!g.aiRunning_) break;

        {
            std::lock_guard<std::mutex> lk(g.worldMutex_);
            if ((g.state_ != GameState::Exploration &&
                 g.state_ != GameState::Shop) || !g.map_)
                continue;

            Position player      = g.map_->getPlayerPos();
            bool     playerInShop = (g.state_ == GameState::Shop);

            for (int i = 0; i < static_cast<int>(g.worldEnemies_.size()); i++) {
                auto& we = g.worldEnemies_[i];
                if (!we.alive) continue;

                if (playerInShop) {
                    if (we.pos.x == we.spawnPos.x && we.pos.y == we.spawnPos.y)
                        continue;
                    Position next = AI::bfsStep(we.pos, we.spawnPos, *g.map_);
                    if (next.x != we.pos.x || next.y != we.pos.y)
                        we.pos = next;
                    continue;
                }

                int dist = std::max(std::abs(we.pos.x - player.x),
                                    std::abs(we.pos.y - player.y));
                if (dist > 10) continue;

                Position next = AI::bfsStep(we.pos, player, *g.map_);

                if (g.shopExists_ && g.isInShopRoom(next)) continue;

                if (next.x == player.x && next.y == player.y) {
                    if (g.pendingCombatEnemy_ < 0)
                        g.pendingCombatEnemy_ = i;
                    continue;
                }

                bool occupied = false;
                for (int j = 0; j < static_cast<int>(g.worldEnemies_.size()); j++) {
                    if (j == i || !g.worldEnemies_[j].alive) continue;
                    if (g.worldEnemies_[j].pos.x == next.x &&
                        g.worldEnemies_[j].pos.y == next.y) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied && (next.x != we.pos.x || next.y != we.pos.y))
                    we.pos = next;
            }
        }

        g.pendingRedraw_.store(true, std::memory_order_release);
    }
}
