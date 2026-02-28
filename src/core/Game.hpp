#pragma once

#include "GameState.hpp"
#include "MenuPhase.hpp"
#include "ShopItem.hpp"
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include "combat/CombatSystem.hpp"
#include "world/Map.hpp"
#include "world/BSPDungeon.hpp"
#include "ui/HudLayout.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// Central game class. Owns the main loop and all subsystems.
class Game {
public:
    Game();
    ~Game();

    void run();

private:
    GameState     state_;
    MenuPhase     menuPhase_;
    bool          running_;

    // Menu state
    int           menuSelection_;   // title: 0=Nueva Partida, 1=Salir
    std::string   playerName_;      // being typed in NameInput
    int           classSelection_;  // 0=Warrior, 1=Mage, 2=Ranger
    int           hudSelection_;    // 0=Sidebar, 1=Bottom

    HudLayout     hudLayout_;

    std::unique_ptr<Player>        player_;
    std::unique_ptr<Map>           map_;
    std::unique_ptr<CombatSystem>  combat_;
    bool                           combatShowingArts_;
    int                            combatArtSelection_;

    // World enemies
    struct WorldEnemy {
        Position  pos;
        Position  spawnPos;
        EnemyType type;
        bool      alive;
    };
    std::vector<WorldEnemy>  worldEnemies_;
    int                      combatWorldEnemyIdx_;

    // Loot types pre-assigned to each chest at spawn time
    enum class ChestLoot { Coins, Item, Key };

    struct WorldChest {
        Position  pos;
        bool      opened;
        ChestLoot loot;
        int       coins;  // if loot == Coins
        Item      item;   // if loot == Item
    };
    std::vector<WorldChest>  worldChests_;

    // Locked door and stairs
    Position  lockedDoorPos_;
    bool      lockedDoorExists_;
    bool      lockedDoorOpen_;
    Position  stairsPos_;

    // Inventory
    int                     inventorySelection_{0};

    // Shop
    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};
    BSPDungeon::Room        shopRoom_{};
    bool                    shopExists_{false};
    Position                shopMerchantPos_{};

    // Exploration message (shown until next key press)
    std::string explorationMsg_;

    // FTXUI
    ftxui::ScreenInteractive screen_;

    void processInput(int key);
    void update();
    ftxui::Element renderDocument();

    void setState(GameState newState);
    void returnToExploration();
    void openChest(WorldChest& chest);
    void tryPlaceSecretRoom(std::vector<Position>& taken, PlayerClass cls);

    // Per-phase input handlers
    void inputTitle(int key);
    void inputNameInput(int key);
    void inputClassSelect(int key);
    void inputHudSelect(int key);
    void inputCombat(int key);
    void inputInventory(int key);
    void inputShop(int key);
    void generateShopStock();
    bool isInShopRoom(Position p) const;

    // AI movement thread
    std::thread          aiThread_;
    std::mutex           worldMutex_;
    std::atomic<bool>    aiRunning_{false};
    int                  pendingCombatEnemy_{-1};
    void aiLoop();
};
