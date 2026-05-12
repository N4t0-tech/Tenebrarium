#include "Dungeon.hpp"
#include "DungeonPopulator.hpp"
#include "ai/Pathfinding.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>

void Dungeon::generate(int floor, PlayerClass cls) {
    std::lock_guard l(mutex_);

    BSPDungeon gen(80, 40);
    gen.generate(map_);

    Position start{gen.getRooms()[0].centerX(), gen.getRooms()[0].centerY()};
    map_.setPlayerPos(start.x, start.y);
    map_.updateFov();

    auto pop = DungeonPopulator::populate(map_, gen.getRooms(), floor, cls);
    enemies_         = std::move(pop.enemies);
    chests_          = std::move(pop.chests);
    stairsPos_       = pop.stairsPos;
    lockedDoorPos_   = pop.lockedDoorPos;
    lockedDoorExists_= pop.lockedDoorExists;
    shopExists_      = pop.shopExists;
    shopRoom_        = pop.shopRoom;
    shopMerchantPos_ = pop.shopMerchantPos;

    // Reset runtime state
    lockedDoorOpen_ = false;
    enemiesKilled = 0;
    chestsOpened  = 0;
    explosionActive = false;
    message.clear();
    messageEndTime = 0.0;
}

int Dungeon::aiTick(bool playerInShop) {
    std::lock_guard l(mutex_);

    Position player = map_.getPlayerPos();

    for (int i = 0; i < static_cast<int>(enemies_.size()); i++) {
        auto& we = enemies_[i];
        if (!we.alive) continue;

        if (playerInShop) {
            if (we.pos.x == we.spawnPos.x && we.pos.y == we.spawnPos.y)
                continue;
            Position next = AI::bfsStep(we.pos, we.spawnPos, map_);
            if (next.x != we.pos.x || next.y != we.pos.y)
                we.pos = next;
            continue;
        }

        int dist = std::max(std::abs(we.pos.x - player.x),
                            std::abs(we.pos.y - player.y));
        if (dist > 10) continue;

        Position next = AI::bfsStep(we.pos, player, map_);

        if (shopExists_ &&
            next.x >= shopRoom_.x && next.x < shopRoom_.x + shopRoom_.w &&
            next.y >= shopRoom_.y && next.y < shopRoom_.y + shopRoom_.h)
            continue;

        if (next.x == player.x && next.y == player.y)
            return i;

        bool occupied = false;
        for (int j = 0; j < static_cast<int>(enemies_.size()); j++) {
            if (j == i || !enemies_[j].alive) continue;
            if (enemies_[j].pos.x == next.x && enemies_[j].pos.y == next.y) {
                occupied = true;
                break;
            }
        }
        if (!occupied && (next.x != we.pos.x || next.y != we.pos.y))
            we.pos = next;
    }

    return -1;
}

bool Dungeon::allEnemiesDead() const {
    std::lock_guard l(mutex_);
    return std::all_of(enemies_.begin(), enemies_.end(),
        [](const WorldEnemy& e) { return !e.alive; });
}

void Dungeon::markEnemyDead(int idx) {
    std::lock_guard l(mutex_);
    if (idx >= 0 && idx < static_cast<int>(enemies_.size()))
        enemies_[idx].alive = false;
}

void Dungeon::openLockedDoor() {
    std::lock_guard l(mutex_);
    lockedDoorOpen_ = true;
}
