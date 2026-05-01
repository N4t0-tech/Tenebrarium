#pragma once

#include "GameState.hpp"
#include "MenuPhase.hpp"
#include "ShopItem.hpp"
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include "combat/CombatSystem.hpp"
#include "world/Map.hpp"
#include "world/BSPDungeon.hpp"
#include "world/WorldObjects.hpp"
#include "ui/HudLayout.hpp"
#include "ui/TerminalScreen.hpp"
#include "quests/Quest.hpp"
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class GameSerializer;
class EnemyAI;

class Game {
    friend class GameSerializer;
    friend class EnemyAI;
public:
    Game();
    ~Game();

    void run();

private:
    GameState     state_;
    MenuPhase     menuPhase_;
    bool          quitRequested_;

    // Menu state
    int           menuSelection_;
    std::string   playerName_;
    int           classSelection_;
    int           hudSelection_;

    HudLayout     hudLayout_;

    std::unique_ptr<Player>        player_;
    std::unique_ptr<Map>           map_;
    std::unique_ptr<CombatSystem>  combat_;
    bool                           combatShowingArts_;
    int                            combatArtSelection_;
    int                            combatFlashIdx_;
    double                         combatFlashEndTime_;

    // World enemies
    std::vector<WorldEnemy>  worldEnemies_;
    int                      combatWorldEnemyIdx_;

    std::vector<WorldChest>  worldChests_;

    // Locked door and stairs
    Position  lockedDoorPos_;
    bool      lockedDoorExists_;
    bool      lockedDoorOpen_;
    Position  stairsPos_;

    // Inventory
    int       inventorySelection_{0};

    // Shop
    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};
    BSPDungeon::Room        shopRoom_{};
    bool                    shopExists_{false};
    Position                shopMerchantPos_{};

    // Exploration message
    std::string explorationMsg_;

    // Quest system
    std::vector<Quest> quests_;
    int                questLogSelection_{0};
    int                enemiesKilled_{0};
    int                chestsOpened_{0};

    int  mapZoom_{1};
    bool victory_{false};

    // Bomb explosion effect
    std::vector<Position> explosionTiles_;
    int                   explosionFrame_{0};
    double                explosionEndTime_{0.0};

    void saveGame();
    bool loadGame();
    bool hasSave() const;
    std::string savePath() const;
    void saveSettings() const;
    void loadSettings();

    void dispatchInput(int key);
    void processInput();
    void update();
    void render(TerminalScreen& scr);

    void setState(GameState newState);
    void returnToExploration();
    void openChest(WorldChest& chest);

    // Per-phase input handlers
    void inputTitle(int key);
    void inputCredits(int key);
    void inputQuitDialog(int key);
    void inputNameInput(int key);
    void inputClassSelect(int key);
    void inputHudSelect(int key);
    void inputCombat(int key);
    void inputInventory(int key);
    void inputShop(int key);
    void inputQuestLog(int key);
    void generateShopStock();
    bool isInShopRoom(Position p) const;
    void useBomb();

    void initQuests();
    void checkQuestProgress();

    // AI movement thread
    std::thread          aiThread_;
    std::mutex           worldMutex_;
    std::atomic<bool>    aiRunning_{false};
    std::atomic<bool>    pendingRedraw_{false};
    int                  pendingCombatEnemy_{-1};
    void aiLoop();
};
