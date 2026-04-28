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

    // Abre la ventana Raylib, carga fuentes/shader y ejecuta el loop hasta salir.
    void run();

private:
    GameState     state_;
    MenuPhase     menuPhase_;   // sub-fase dentro de MainMenu
    bool          quitRequested_;

    // Estado del menú principal y flujo de creación de personaje
    int           menuSelection_;
    std::string   playerName_;
    int           classSelection_;
    int           hudSelection_;

    HudLayout     hudLayout_;   // Sidebar o Bottom — elegido en HudSelect

    std::unique_ptr<Player>        player_;
    std::unique_ptr<Map>           map_;
    std::unique_ptr<CombatSystem>  combat_;
    bool                           combatShowingArts_;  // true = submenú de habilidades abierto
    int                            combatArtSelection_;
    // Flash visual al impactar: índice del enemigo que parpadea y hasta cuándo
    int                            combatFlashIdx_;
    double                         combatFlashEndTime_;

    // Lista de enemigos vivos en el mundo; usada por el renderer y la IA
    // INVARIANTE: acceder/modificar siempre bajo worldMutex_
    std::vector<WorldEnemy>  worldEnemies_;
    int                      combatWorldEnemyIdx_;  // índice en worldEnemies_ del combate activo (-1 si ninguno)

    std::vector<WorldChest>  worldChests_;

    // La puerta bloqueada se abre cuando todos los enemigos del piso mueren
    Position  lockedDoorPos_;
    bool      lockedDoorExists_;
    bool      lockedDoorOpen_;
    Position  stairsPos_;

    int       inventorySelection_{0};
    int       inventoryTab_{0};       // 0 = Pociones, 1 = Equipo
    std::string inventoryMsg_;        // feedback dentro del inventario

    // Stock de la tienda generado una sola vez por piso (ver generateShopStock)
    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};
    BSPDungeon::Room        shopRoom_{};
    bool                    shopExists_{false};
    Position                shopMerchantPos_{};

    // Mensaje flotante en exploración (se limpia al mover o al nuevo turno)
    std::string explorationMsg_;

    std::vector<Quest> quests_;
    int                questLogSelection_{0};
    int                enemiesKilled_{0};  // contadores para objetivos de misiones
    int                chestsOpened_{0};

    int  mapZoom_{1};   // 1–3; escala la celda del mapa overlay
    bool victory_{false};

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
    void generateShopStock();
    bool isInShopRoom(Position p) const;

    void initQuests();
    void checkQuestProgress();

    // Thread de IA: mueve enemigos cada 600 ms (ver EnemyAI::run).
    // Comunicación con el hilo principal mediante worldMutex_ + pendingRedraw_.
    // pendingCombatEnemy_: cuando la IA detecta colisión con el jugador, pone aquí
    // el índice del enemigo; el hilo principal lo recoge en el próximo frame.
    std::thread          aiThread_;
    std::mutex           worldMutex_;
    std::atomic<bool>    aiRunning_{false};
    std::atomic<bool>    pendingRedraw_{false};
    int                  pendingCombatEnemy_{-1};
    void aiLoop();
};
