#pragma once

// Game es la clase central: contiene el loop principal, la máquina de estados,
// todos los objetos del mundo y la lógica de transición entre pantallas.
// GameSerializer y EnemyAI son friend para acceder a miembros privados sin
// getters adicionales.

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

    // Abre la ventana Raylib, carga fuentes/shader y ejecuta el loop hasta salir.
    void run();

private:
    std::atomic<GameState>  state_{GameState::MainMenu};
    MenuPhase     menuPhase_;
    bool          quitRequested_;

    // Estado del menú principal y flujo de creación de personaje
    int           menuSelection_;
    std::string   playerName_;
    int           classSelection_;
    int           hudSelection_;
    int           settingsSelection_{0};

    HudLayout     hudLayout_;   // Sidebar o Bottom — elegido en HudSelect

    std::unique_ptr<Player>        player_;
    std::unique_ptr<Dungeon>       dungeon_;
    std::unique_ptr<CombatSystem>  combat_;
    bool                           combatShowingArts_;  // true = submenú de habilidades abierto
    int                            combatArtSelection_;
    // Flash visual al impactar: índice del enemigo que parpadea y hasta cuándo
    int                            combatFlashIdx_;
    double                         combatFlashEndTime_;

    int           combatWorldEnemyIdx_{-1};
    bool          mimicCombat_{false};

    std::atomic<int>  pendingCombatEnemy_{-1};

    int  mapZoom_{1};
    bool shaderEnabled_{true};
    bool victory_{false};

    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};

    std::vector<Quest> quests_;
    int                questLogSelection_{0};

    int                   inventorySelection_{0};

    void saveGame();
    bool loadGame();
    bool hasSave() const;
    std::string savePath() const;
    void saveSettings() const;
    void loadSettings();

    // processInput() convierte keycodes de Raylib a enteros internos y llama dispatchInput.
    // dispatchInput() enruta al handler del estado actual.
    void dispatchInput(int key);
    void processInput();
    void update();
    void render(TerminalScreen& scr);

    // setState() es el punto único de transición: limpia estado obsoleto y genera
    // el nuevo (mapa, enemigos, combat, etc.) según el estado destino.
    void setState(GameState newState);
    // returnToExploration() vuelve al mapa sin regenerarlo (post-combate o ESC en QuitDialog)
    void returnToExploration();
    void openChest(WorldChest& chest);

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

    // Thread de IA: mueve enemigos cada 600 ms (ver EnemyAI::run).
    // Comunicación con el hilo principal mediante worldMutex_ + pendingRedraw_.
    // pendingCombatEnemy_: cuando la IA detecta colisión con el jugador, pone aquí
    // el índice del enemigo; el hilo principal lo recoge en el próximo frame.
    std::thread          aiThread_;
    std::atomic<bool>    aiRunning_{false};
    std::atomic<bool>    pendingRedraw_{false};
    void aiLoop();
};
