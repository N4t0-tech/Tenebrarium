#pragma once

#include "Map.hpp"
#include "WorldObjects.hpp"
#include "BSPDungeon.hpp"
#include "entities/Player.hpp"
#include <vector>
#include <mutex>

class GameSerializer;

class Dungeon {
    friend class GameSerializer;
public:
    Dungeon() = default;

    void generate(int floor, PlayerClass cls);

    // ── AI thread (locks internally) ──
    // Runs the AI tick for all enemies. Returns enemy index to start combat with,
    // or -1 if no combat triggered.
    int aiTick(bool playerInShop);

    // ── Main-thread queries (lock internally) ──
    bool allEnemiesDead() const;
    void markEnemyDead(int idx);
    void openLockedDoor();

    // ── Locked handle for batch access (render, collision, save/load) ──
    struct Lock {
        std::lock_guard<std::mutex> guard;
        Dungeon& self;

        Map&       map()       { return self.map_; }
        const Map& map() const { return self.map_; }

        auto&       enemies()       { return self.enemies_; }
        const auto& enemies() const { return self.enemies_; }

        auto&       chests()       { return self.chests_; }
        const auto& chests() const { return self.chests_; }

        auto&       torches()       { return self.torches_; }
        const auto& torches() const { return self.torches_; }

        // Convenience
        Position  playerPos()     const { return self.map_.getPlayerPos(); }
        void      setPlayerPos(int x, int y) { self.map_.setPlayerPos(x, y); }
        bool      isWalkable(int x, int y) const { return self.map_.isWalkable(x, y); }
        bool      isSecretWall(int x, int y) const { return self.map_.isSecretWall(x, y); }

        // World state
        Position  stairsPos()     const { return self.stairsPos_; }
        Position  lockedDoorPos() const { return self.lockedDoorPos_; }
        bool      lockedDoorExists() const { return self.lockedDoorExists_; }
        bool      lockedDoorOpen() const { return self.lockedDoorOpen_; }
        bool      shopExists()     const { return self.shopExists_; }
        Position  shopMerchantPos() const { return self.shopMerchantPos_; }

        bool isInShopRoom(Position p) const {
            if (!self.shopExists_) return false;
            return p.x >= self.shopRoom_.x && p.x < self.shopRoom_.x + self.shopRoom_.w &&
                   p.y >= self.shopRoom_.y && p.y < self.shopRoom_.y + self.shopRoom_.h;
        }

        const auto& shopRoom() const { return self.shopRoom_; }
        void lockedDoorOpen(bool v) { self.lockedDoorOpen_ = v; }
    };

    Lock lock() { return {std::lock_guard(mutex_), *this}; }

    // ── Main-thread only (AI never touches these) ──
    int     enemiesKilled = 0;
    int     chestsOpened  = 0;
    bool    explosionActive   = false;
    double  explosionEndTime  = 0.0;
    int     explosionX = 0, explosionY = 0;
    std::string message;
    double      messageEndTime = 0.0;

private:
    mutable std::mutex mutex_;
    Map                    map_{80, 40};
    std::vector<WorldEnemy> enemies_;
    std::vector<WorldChest> chests_;
    std::vector<WorldTorch> torches_;
    Position               stairsPos_{};
    Position               lockedDoorPos_{};
    bool                   lockedDoorExists_ = false;
    bool                   lockedDoorOpen_   = false;
    BSPDungeon::Room       shopRoom_{};
    bool                   shopExists_ = false;
    Position               shopMerchantPos_{};
};
