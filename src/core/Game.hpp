#pragma once

#include "GameState.hpp"
#include "MenuPhase.hpp"
#include "ShopItem.hpp"
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include "combat/CombatSystem.hpp"
#include "world/Dungeon.hpp"
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
    std::atomic<GameState>  state_{GameState::MainMenu};
    MenuPhase     menuPhase_;
    bool          quitRequested_;

    // Menu state
    int           menuSelection_;
    std::string   playerName_;
    int           classSelection_;
    int           hudSelection_;
    int           settingsSelection_{0};

    HudLayout     hudLayout_;

    std::unique_ptr<Player>        player_;
    std::unique_ptr<Dungeon>       dungeon_;
    std::unique_ptr<CombatSystem>  combat_;
    bool                           combatShowingArts_;
    int                            combatArtSelection_;
    int                            combatFlashIdx_;
    double                         combatFlashEndTime_;

    // World enemy index that triggered current combat
    int           combatWorldEnemyIdx_{-1};
    bool          mimicCombat_{false};

    // AI communication (written by AI thread, read by main thread)
    std::atomic<int>  pendingCombatEnemy_{-1};

    // HUD layout
    int  mapZoom_{1};
    bool shaderEnabled_{true};
    bool victory_{false};

    // Shop stock (UI-only, not world state)
    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};

    // Quest tracking
    std::vector<Quest> quests_;
    int                questLogSelection_{0};

    int                   inventorySelection_{0};

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
    void inputSettings(int key);
    void generateShopStock();
    bool isInShopRoom(Dungeon::Lock& acc, Position p) const;
    void useBomb(Dungeon::Lock& acc);

    void initQuests();
    void checkQuestProgress();

    // AI movement thread
    std::thread          aiThread_;
    std::atomic<bool>    aiRunning_{false};
    std::atomic<bool>    pendingRedraw_{false};
    void aiLoop();
};
