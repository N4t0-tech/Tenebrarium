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
#include "world/Village.hpp"
#include "world/DayCycle.hpp"
#include "ui/HudLayout.hpp"
#include "ui/TerminalScreen.hpp"
#include "quests/Quest.hpp"
#include "bestiary/Bestiary.hpp"
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
    int           classSelectCategory_{0};          // 0=Física 1=Mágica 2=Distancia
    int           hudSelection_;
    int           settingsSelection_{0};

    HudLayout     hudLayout_;   // Sidebar o Bottom — elegido en HudSelect

    std::unique_ptr<Player>              player_;
    std::shared_ptr<Dungeon>             dungeon_;
    std::unique_ptr<CombatSystem>        combat_;

    // Pueblo hub (estado Village). villageMenu_ controla el submenú abierto
    // (forja / entrenamiento) mientras el jugador camina por el mapa.
    std::unique_ptr<VillageLayout> village_;
    VillageMenu  villageMenu_{VillageMenu::None};
    bool         villageConfirmDescend_{false};  // pisó la escalera, espera ENTER
    float        villageHour_{kStartHour};       // hora del pueblo (0–24), ver DayCycle.hpp
    int          villageSelection_{0};
    std::string  villageMessage_;
    double       villageMessageEndTime_{0.0};
    bool                           combatShowingArts_;  // true = submenú de habilidades abierto
    int                            combatArtSelection_;
    // Flash visual al impactar: índice del enemigo que parpadea y hasta cuándo
    int                            combatFlashIdx_;
    double                         combatFlashEndTime_;

    int                combatWorldEnemyIdx_{-1};
    std::vector<int>   combatEnemyIndices_;
    bool               mimicCombat_{false};

    std::atomic<int>  pendingCombatEnemy_{-1};

    int  mapZoom_{1};
    int  scrollTick_{0};
    bool shaderEnabled_{true};
    bool arrowMovement_{false};   // true: moverse con las flechas en lugar de WASD
    bool fullscreen_{false};
    bool wasFullscreen_{false};
    int  windowedW_{1280};
    int  windowedH_{800};
    int  windowedX_{0};
    int  windowedY_{0};
    bool victory_{false};
    bool secretEasterEgg_{false};

    // Árbol de habilidades
    GameState     preSkillTreeState_{GameState::Exploration};
    int           skillTreeTierCursor_{0};
    int           skillTreeChoiceCursor_{0};

    // Apuntado en el mapa (trampas de la Arquera, hechizos del Mago).
    // Mientras se apunta, la IA queda en pausa.
    enum class AimKind { Trap, Spell, Throw };
    std::atomic<bool> aiming_{false};
    AimKind           aimKind_{AimKind::Trap};
    Position          aimCursor_{};
    int               spellSelection_{0};   // índice en kMapSpells

    // Hechizos en vuelo: avanzan por casillas y se resuelven al chocar con un
    // enemigo o al llegar al final de la trayectoria. Solo hilo principal.
    struct Projectile {
        Position              from;
        std::vector<Position> path;      // casillas tras el jugador hasta el impacto
        double                start;
        float                 speed;     // casillas por segundo
        int                   glyph;
        Color                 color;
        int                   spellIdx{-1};  // índice en kMapSpells, o -1
        int                   corpseIdx{-1}; // cadáver lanzado (índice en enemies), o -1
        int                   reached{0};    // casillas ya comprobadas
    };
    std::vector<Projectile> projectiles_;

    // Regeneración de maná del Mago al explorar; acumula fracciones entre frames.
    float manaRegenAcc_{0.0f};
    void updateManaRegen();

    // Geometría del mapa en pantalla (píxeles), recalculada cada frame en run().
    // Sirve para convertir entre casillas del mapa y píxeles (mouse, explosiones).
    struct MapViewport {
        int originX{0}, originY{0};   // píxel de la esquina superior izquierda del mapa
        int cellW{1}, cellH{1};       // tamaño de casilla (incluye zoom)
        int viewW{0}, viewH{0};       // casillas visibles
        float camX{0}, camY{0};       // casilla del mapa en la esquina superior izquierda (suavizada)
    } mapViewport_;

    // Cámara suavizada: persigue al jugador en lugar de saltar casilla a casilla.
    const Map* camMap_{nullptr};
    int        camViewW_{0}, camViewH_{0};
    void updateCamera(float dt);

    bool   shovelDigging_{false};
    double shovelDigEndTime_{0.0};

    double screenshotMsgEndTime_{0.0};
    int screenshotCounter_{0};

    // Movimiento con tecla mantenida: ritmo propio en lugar de la repetición del SO.
    int    heldMoveKey_{0};
    int    heldMoveRl_{0};
    double nextMoveTime_{0.0};

    std::vector<ShopItem>   shopStock_;
    int                     shopSelection_{0};
    bool                    shopSellMode_{false};
    int                     shopSellSelection_{0};

    std::vector<Quest> quests_;
    int                questLogSelection_{0};

    std::array<BestiaryEntry, kBestiaryEntryCount> bestiary_;
    int                bestiarySelection_{0};

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
    bool processHeldMovement();
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
    void inputBestiary(int key);
    void inputSettings(int key);
    void inputVillage(int key);
    void inputSkillTree(int key);
    void generateShopStock();
    void useShovel();
    void beginAim(AimKind kind);
    void endAim(const std::string& msg = "", double duration = 1.5);
    void inputAiming(int key);
    void inputAimMouse();
    std::string aimHint() const;
    std::vector<int> unlockedMapSpells() const;
    // Píxel de pantalla → casilla del mapa. false si cae fuera del mapa.
    bool screenToMap(Vector2 px, Position& out) const;
    std::string trapPlacementError(const Dungeon::Lock& acc, Position p) const;
    std::string spellTargetError(const Dungeon::Lock& acc, Position p) const;
    std::vector<Position> spellPath(const Dungeon::Lock& acc, Position target) const;
    void castSpell(Position target);
    void updateProjectiles();
    void resolveSpellImpact(int spellIdx, Position at, bool alwaysReport);

    // Cadáveres del Guerrero: G levanta uno cercano; con uno en brazos, G apunta y lo lanza.
    bool canThrowCorpses() const;
    int  carriedCorpse(const Dungeon::Lock& acc) const;   // índice en enemies o -1
    void pickUpOrAimCorpse();
    std::string throwTargetError(const Dungeon::Lock& acc, Position p) const;
    void throwCorpse(Position target);
    void resolveCorpseImpact(int corpseIdx, Position at);
    void resolveTrapTriggers();
    void appendAimEntities(const Dungeon::Lock& acc, std::vector<MapEntity>& out) const;

    // Daño en área a enemigos del mapa: % de vida máxima + daño plano.
    struct AreaHit {
        int hits{0}, frozen{0}, damage{0};
        std::vector<std::string> killed;
        std::string loot;
    };
    AreaHit damageEnemiesAround(Dungeon::Lock& acc, Position c, int radius,
                                int pctNormal, int pctBoss, int flat, int freezeTicks = 0);
    static std::string describeHit(const std::string& prefix, const AreaHit& hit);
    void triggerExplosion(Position c, int radius, int style);
    // Aplica XP, bestiario y botín por un enemigo del mapa abatido. Devuelve el
    // nombre del objeto obtenido o "" si no hubo botín.
    std::string rewardEnemyKill(WorldEnemy& we);
    // Enruta un mensaje transitorio al mapa activo (mazmorra o pueblo).
    void showMessage(const std::string& msg, double duration = 2.0);

    void initQuests();
    void initBestiary();
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
