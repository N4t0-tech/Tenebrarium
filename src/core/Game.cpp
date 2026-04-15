#include "Game.hpp"
#include "core/Assets.hpp"
#include "ui/Renderer.hpp"
#include "entities/Enemy.hpp"
#include <raylib.h>
#include <queue>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <filesystem>

// Keycodes internos del juego (evitan conflicto con KeyboardKey de Raylib)
static constexpr int GKEY_UP = 0x1001;
static constexpr int GKEY_DOWN = 0x1002;
static constexpr int GKEY_LEFT = 0x1003;
static constexpr int GKEY_RIGHT = 0x1004;
static constexpr int GKEY_BACKSPACE = 0x1005;

// Forward declarations of file-local helpers (defined near setState)
static char glyphForEnemy(EnemyType t);
static EnemyType pickEnemyType(int floor);
static std::unique_ptr<Enemy> makeEnemy(EnemyType t, int floor, bool isBoss = false);
static int xpForEnemy(EnemyType t, int floor);
static Item pickWeapon(PlayerClass cls, int floor);
static Item pickArmor(PlayerClass cls, int floor);
static Item pickPotion(int floor);

Game::Game()
    : state_(GameState::MainMenu),
      menuPhase_(MenuPhase::Title),
      quitRequested_(false),
      menuSelection_(0),
      classSelection_(0),
      hudSelection_(0),
      hudLayout_(HudLayout::Sidebar),
      combatShowingArts_(false),
      combatArtSelection_(0),
      combatFlashIdx_(-1),
      combatFlashEndTime_(0.0),
      combatWorldEnemyIdx_(-1),
      lockedDoorPos_({0, 0}),
      lockedDoorExists_(false),
      lockedDoorOpen_(false),
      stairsPos_({0, 0})
{
    aiRunning_ = true;
    aiThread_ = std::thread(&Game::aiLoop, this);
}

Game::~Game()
{
    aiRunning_ = false;
    if (aiThread_.joinable())
        aiThread_.join();
}

void Game::run()
{
    const int SCREEN_W = 1280;
    const int SCREEN_H = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "Tenebrarium");
    SetTargetFPS(60);
    SetExitKey(0); // desactivar cierre con ESC (lo manejamos nosotros)

    // Ruta de assets relativa al ejecutable (funciona sin importar dónde esté el binario)
    assetsDir() = std::string(GetApplicationDirectory()) + "assets/";

    // Icono de ventana
    Image icon = LoadImage((assetsDir() + "icon.png").c_str());
    if (icon.data)
    {
        SetWindowIcon(icon);
        UnloadImage(icon);
    }

    // Centrar en el monitor principal (índice 0)
    int monX = GetMonitorPosition(0).x;
    int monY = GetMonitorPosition(0).y;
    int monW = GetMonitorWidth(0);
    int monH = GetMonitorHeight(0);
    SetWindowPosition(monX + (monW - SCREEN_W) / 2,
                      monY + (monH - SCREEN_H) / 2);

    // Codepoints necesarios: ASCII + dibujo de caja + bloques + algunos CP437
    std::vector<int> codepoints;
    // ASCII imprimible
    for (int i = 32; i < 127; i++)
        codepoints.push_back(i);
    // Dibujo de caja: ─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼  (U+2500–U+257F)
    for (int i = 0x2500; i <= 0x257F; i++)
        codepoints.push_back(i);
    // Elementos de bloque: █ ░ ▒ ▓ ▀ ▄ etc. (U+2580–U+259F)
    for (int i = 0x2580; i <= 0x259F; i++)
        codepoints.push_back(i);
    // Símbolos misceláneos usados por CP437 (flechas, caras, etc.)
    int extras[] = {
        0x263A,
        0x263B,
        0x2665,
        0x2666,
        0x2663,
        0x2660,
        0x2022,
        0x25D8,
        0x25CB,
        0x25D9,
        0x2642,
        0x2640,
        0x266A,
        0x266B,
        0x263C,
        0x25BA,
        0x25C4,
        0x2195,
        0x203C,
        0x2191,
        0x2193,
        0x2192,
        0x2190,
        0x221F,
        0x2194,
        0x25B2,
        0x25BC,
        0x2302,
        0x2310,
        0x00AC,
        0x00BD,
        0x00BC,
        0x00AB,
        0x00BB,
        0x03B1,
        0x00DF,
        0x0393,
        0x03C0,
        0x03A3,
        0x03C3,
        0x03C4,
        0x03A6,
        0x0398,
        0x03A9,
        0x03B4,
        0x221E,
        0x03C6,
        0x03B5,
        0x2229,
        0x2261,
        0x00B1,
        0x2265,
        0x2264,
        0x2320,
        0x2321,
        0x00F7,
        0x2248,
        0x00B0,
        0x2219,
        0x00B7,
        0x221A,
        0x207F,
        0x00B2,
        0x25A0,
        // letras latinas extendidas comunes
        0x00C7,
        0x00FC,
        0x00E9,
        0x00E2,
        0x00E4,
        0x00E0,
        0x00E5,
        0x00E7,
        0x00EA,
        0x00EB,
        0x00E8,
        0x00EF,
        0x00EE,
        0x00EC,
        0x00C4,
        0x00C5,
        0x00C9,
        0x00E6,
        0x00C6,
        0x00F4,
        0x00F6,
        0x00F2,
        0x00FB,
        0x00F9,
        0x00FF,
        0x00D6,
        0x00DC,
        0x00A2,
        0x00A3,
        0x00A5,
        0x00E1,
        0x00ED,
        0x00F3,
        0x00FA,
        0x00F1,
        0x00D1,
        0x00AA,
        0x00BA,
        0x00BF,
        0x00A1,
        // doble línea CP437
        0x2550,
        0x2551,
        0x2552,
        0x2553,
        0x2554,
        0x2555,
        0x2556,
        0x2557,
        0x2558,
        0x2559,
        0x255A,
        0x255B,
        0x255C,
        0x255D,
        0x255E,
        0x255F,
        0x2560,
        0x2561,
        0x2562,
        0x2563,
        0x2564,
        0x2565,
        0x2566,
        0x2567,
        0x2568,
        0x2569,
        0x256A,
        0x256B,
        0x256C,
        // círculos para PA ● ○
        0x25CF,
        0x25CB,
    };
    for (int cp : extras)
        codepoints.push_back(cp);

    Font font = LoadFontEx((assetsDir() + "fonts/mono.ttf").c_str(), 18,
                           codepoints.data(), static_cast<int>(codepoints.size()));
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);

    // Calcular tamaño de celda
    Vector2 gs = MeasureTextEx(font, "M", 18, 0);
    int cellW = static_cast<int>(gs.x);
    int cellH = static_cast<int>(gs.y) + 2;

    // Shader CRT
    Shader crtShader = LoadShader(0, (assetsDir() + "shaders/crt.frag").c_str());
    int resLoc = GetShaderLocation(crtShader, "resolution");

    // Render texture offscreen
    RenderTexture2D renderTarget = LoadRenderTexture(SCREEN_W, SCREEN_H);
    int rtW = SCREEN_W, rtH = SCREEN_H;

    while (!WindowShouldClose() && !quitRequested_)
    {
        // Recalcular grid si la ventana cambió de tamaño
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        int cols = screenW / cellW;
        int rows = screenH / cellH;
        if (cols < 1) cols = 1;
        if (rows < 1) rows = 1;

        // Recrear render texture si la ventana cambió de tamaño
        if (IsWindowResized() && (screenW != rtW || screenH != rtH)) {
            UnloadRenderTexture(renderTarget);
            renderTarget = LoadRenderTexture(screenW, screenH);
            rtW = screenW;
            rtH = screenH;
        }

        // Actualizar uniform de resolución
        float res[2] = { (float)rtW, (float)rtH };
        SetShaderValue(crtShader, resLoc, res, SHADER_UNIFORM_VEC2);

        // Procesar IA pending
        if (pendingRedraw_.load(std::memory_order_acquire))
        {
            pendingRedraw_.store(false, std::memory_order_release);
            std::lock_guard<std::mutex> lk(worldMutex_);
            if (pendingCombatEnemy_ >= 0 && state_ == GameState::Exploration)
            {
                combatWorldEnemyIdx_ = pendingCombatEnemy_;
                pendingCombatEnemy_ = -1;
                setState(GameState::Combat);
            }
        }

        processInput();
        update();

        TerminalScreen scr(cols, rows, cellW, cellH, font);
        scr.clear();
        render(scr);

        // 1) Renderizar juego al texture offscreen
        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);
        scr.render();
        EndTextureMode();

        // 2) Dibujar texture con shader CRT
        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(crtShader);
        // La RenderTexture está volteada verticalmente en Raylib
        DrawTexturePro(
            renderTarget.texture,
            { 0, 0, (float)rtW, -(float)rtH },
            { 0, 0, (float)screenW, (float)screenH },
            { 0, 0 }, 0.0f, WHITE
        );
        EndShaderMode();
        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    UnloadShader(crtShader);
    UnloadFont(font);
    CloseWindow();
}

void Game::processInput()
{
    // Raylib keycode → código interno del juego
    // Los valores numéricos son los de KeyboardKey en raylib.h
    struct
    {
        int rl;
        int g;
    } mapping[] = {
        {265, GKEY_UP},        // KEY_UP
        {264, GKEY_DOWN},      // KEY_DOWN
        {263, GKEY_LEFT},      // KEY_LEFT
        {262, GKEY_RIGHT},     // KEY_RIGHT
        {257, '\n'},           // KEY_ENTER
        {335, '\n'},           // KEY_KP_ENTER
        {256, 27},             // KEY_ESCAPE
        {258, '\t'},           // KEY_TAB
        {259, GKEY_BACKSPACE}, // KEY_BACKSPACE
        {32, ' '},             // KEY_SPACE
    };
    for (auto &m : mapping)
    {
        if (IsKeyPressed(m.rl))
        {
            dispatchInput(m.g);
            return;
        }
    }
    // Carácter Unicode (letras, números, etc.)
    int cp = GetCharPressed();
    if (cp > 0)
        dispatchInput(cp);
}

void Game::dispatchInput(int key)
{
    switch (state_)
    {
    case GameState::MainMenu:
        switch (menuPhase_)
        {
        case MenuPhase::Title:
            inputTitle(key);
            break;
        case MenuPhase::Credits:
            inputCredits(key);
            break;
        case MenuPhase::NameInput:
            inputNameInput(key);
            break;
        case MenuPhase::ClassSelect:
            inputClassSelect(key);
            break;
        case MenuPhase::HudSelect:
            inputHudSelect(key);
            break;
        }
        break;
    case GameState::Exploration:
    {
        explorationMsg_.clear();
        if (key == 'q' || key == 'Q')
        {
            menuSelection_ = 0;
            setState(GameState::QuitDialog);
            break;
        }
        if (!map_)
            break;
        Position pos = map_->getPlayerPos();

        // Use a potion
        if (key == 'p' || key == 'P')
        {
            int healed = player_->useConsumable();
            if (healed > 0)
                explorationMsg_ = "Usas una pocion: +" + std::to_string(healed) + " HP!";
            else
                explorationMsg_ = "No tienes pociones.";
            break;
        }

        // Open inventory
        if (key == 'i' || key == 'I')
        {
            inventorySelection_ = 0;
            setState(GameState::Inventory);
            break;
        }

        // Open quest log
        if (key == 'm' || key == 'M')
        {
            questLogSelection_ = 0;
            setState(GameState::QuestLog);
            break;
        }

        // Search adjacent tiles for secret walls
        if (key == 'e' || key == 'E')
        {
            const int sdx[] = {0, 0, -1, 1};
            const int sdy[] = {-1, 1, 0, 0};
            bool found = false;
            for (int i = 0; i < 4; i++)
            {
                int ax = pos.x + sdx[i], ay = pos.y + sdy[i];
                if (map_->isSecretWall(ax, ay))
                {
                    map_->revealSecretWall(ax, ay);
                    map_->updateFov();
                    explorationMsg_ = "Encontraste una sala secreta!";
                    found = true;
                }
            }
            if (!found)
                explorationMsg_ = "No hay nada aqui...";
            break;
        }

        int nx = pos.x, ny = pos.y;
        if (key == GKEY_UP || key == 'w' || key == 'k')
            ny--;
        else if (key == GKEY_DOWN || key == 's' || key == 'j')
            ny++;
        else if (key == GKEY_LEFT || key == 'a' || key == 'h')
            nx--;
        else if (key == GKEY_RIGHT || key == 'd' || key == 'l')
            nx++;
        if (!map_->isWalkable(nx, ny))
            break;

        // Merchant tile → open shop
        if (shopExists_ && nx == shopMerchantPos_.x && ny == shopMerchantPos_.y)
        {
            generateShopStock();
            shopSelection_ = 0;
            setState(GameState::Shop);
            break;
        }

        // Locked door blocks movement
        if (lockedDoorExists_ && !lockedDoorOpen_ &&
            lockedDoorPos_.x == nx && lockedDoorPos_.y == ny)
        {
            if (player_->useKey())
            {
                lockedDoorOpen_ = true;
                explorationMsg_ = "Usas una llave. La puerta se abre y aparecen unas escaleras!";
            }
            else
            {
                explorationMsg_ = "La puerta esta cerrada con llave. Necesitas una llave.";
            }
            break;
        }

        // Enemy collision → combat (protected against AI thread)
        bool triggered = false;
        {
            std::lock_guard<std::mutex> lk(worldMutex_);
            for (int i = 0; i < static_cast<int>(worldEnemies_.size()); i++)
            {
                auto &we = worldEnemies_[i];
                if (we.alive && we.pos.x == nx && we.pos.y == ny)
                {
                    combatWorldEnemyIdx_ = i;
                    triggered = true;
                    break;
                }
            }
            if (!triggered)
            {
                map_->setPlayerPos(nx, ny);
                map_->updateFov();
            }
        }
        if (triggered)
        {
            setState(GameState::Combat);
            break;
        }

        // Chest on new tile
        for (auto &ch : worldChests_)
        {
            if (!ch.opened && ch.pos.x == nx && ch.pos.y == ny)
            {
                openChest(ch);
                break;
            }
        }

        // Stairs on new tile
        if ((!lockedDoorExists_ || lockedDoorOpen_) &&
            stairsPos_.x == nx && stairsPos_.y == ny)
        {
            player_->descendFloor();
            setState(GameState::Exploration);
            explorationMsg_ = "Desciendes al piso " +
                              std::to_string(player_->getDungeonFloor()) + "...";
            saveGame();
        }
        break;
    }
    case GameState::Combat:
        inputCombat(key);
        break;
    case GameState::Inventory:
        inputInventory(key);
        break;
    case GameState::QuestLog:
        inputQuestLog(key);
        break;
    case GameState::GameOver:
        if (key == '\n' || key == '\n')
            setState(GameState::MainMenu);
        break;
    case GameState::Shop:
        inputShop(key);
        break;
    case GameState::QuitDialog:
        inputQuitDialog(key);
        break;
    }
}

void Game::inputTitle(int key)
{
    const bool hs = hasSave();
    const int n = hs ? 4 : 3;
    switch (key)
    {
    case GKEY_UP:
    case GKEY_LEFT:
        menuSelection_ = (menuSelection_ - 1 + n) % n;
        break;
    case GKEY_DOWN:
    case GKEY_RIGHT:
        menuSelection_ = (menuSelection_ + 1) % n;
        break;
    case '\n':
        if (hs)
        {
            if (menuSelection_ == 0)
            {
                loadGame();
            }
            else if (menuSelection_ == 1)
            {
                playerName_.clear();
                menuPhase_ = MenuPhase::NameInput;
            }
            else if (menuSelection_ == 2)
            {
                menuPhase_ = MenuPhase::Credits;
            }
            else
            {
                quitRequested_ = true;
            }
        }
        else
        {
            if (menuSelection_ == 0)
            {
                playerName_.clear();
                menuPhase_ = MenuPhase::NameInput;
            }
            else if (menuSelection_ == 1)
            {
                menuPhase_ = MenuPhase::Credits;
            }
            else
            {
                quitRequested_ = true;
            }
        }
        break;
    case 'q':
    case 'Q':
        quitRequested_ = true;
        break;
    }
}

void Game::inputCredits(int key)
{
    if (key == 27 || key == '\n')
    {
        menuSelection_ = 0;
        menuPhase_ = MenuPhase::Title;
    }
}

void Game::inputQuitDialog(int key)
{
    switch (key) {
        case GKEY_UP:
        case GKEY_LEFT:
            menuSelection_ = (menuSelection_ - 1 + 2) % 2;
            break;
        case GKEY_DOWN:
        case GKEY_RIGHT:
            menuSelection_ = (menuSelection_ + 1) % 2;
            break;
        case '\n':
            if (menuSelection_ == 0)
                setState(GameState::MainMenu);
            else
                quitRequested_ = true;
            break;
        case 27:  // ESC — cancelar
            setState(GameState::Exploration);
            break;
    }
}

void Game::inputNameInput(int key)
{
    if (key == '\n' || key == '\n')
    {
        if (!playerName_.empty())
        {
            classSelection_ = 0;
            menuPhase_ = MenuPhase::ClassSelect;
        }
        return;
    }
    if (key == GKEY_BACKSPACE || key == 127 || key == 8)
    {
        if (!playerName_.empty())
            playerName_.pop_back();
        return;
    }
    if (key == 27)
    {
        menuPhase_ = MenuPhase::Title;
        return;
    }
    if (key >= 32 && key <= 126 && static_cast<int>(playerName_.size()) < 16)
        playerName_ += static_cast<char>(key);
}

void Game::inputClassSelect(int key)
{
    switch (key)
    {
    case GKEY_UP:
        classSelection_ = (classSelection_ + 2) % 3;
        break;
    case GKEY_DOWN:
        classSelection_ = (classSelection_ + 1) % 3;
        break;
    case 27:
        menuPhase_ = MenuPhase::NameInput;
        break;
    case '\n':
        hudSelection_ = 0;
        menuPhase_ = MenuPhase::HudSelect;
        break;
    }
}

void Game::inputHudSelect(int key)
{
    switch (key)
    {
    case GKEY_LEFT:
    case GKEY_RIGHT:
        hudSelection_ = (hudSelection_ + 1) % 2;
        break;
    case 27:
        menuPhase_ = MenuPhase::ClassSelect;
        break;
    case '\n':
    {
        hudLayout_ = (hudSelection_ == 0) ? HudLayout::Sidebar : HudLayout::Bottom;
        PlayerClass cls;
        switch (classSelection_)
        {
        case 0:
            cls = PlayerClass::Warrior;
            break;
        case 1:
            cls = PlayerClass::Mage;
            break;
        default:
            cls = PlayerClass::Ranger;
            break;
        }
        player_ = std::make_unique<Player>(playerName_, cls);
        initQuests();
        setState(GameState::Exploration);
        break;
    }
    }
}

void Game::update()
{
    checkQuestProgress();
}

void Game::initQuests()
{
    quests_.clear();
    enemiesKilled_ = 0;
    chestsOpened_ = 0;

    quests_.push_back({"primer_sangre", "Primer Sangre", "Derrota a tu primer enemigo en el Tenebrarium.", QuestStatus::InProgress, {{"Mata 1 enemigo", false}}, 50, 0});
    quests_.push_back({"cazador", "Cazador", "Los monstruos del Tenebrarium son peligrosos. Demuestra que no te amedrentan.", QuestStatus::InProgress, {{"Mata 5 enemigos", false}}, 150, 20});
    quests_.push_back({"buscador", "Buscador de Tesoros", "Las criptas estan llenas de riquezas olvidadas. Encuentralas.", QuestStatus::InProgress, {{"Abre 3 cofres", false}}, 100, 30});
    quests_.push_back({"descenso", "Descenso a las Profundidades", "Las criaturas mas peligrosas habitan los pisos inferiores.", QuestStatus::InProgress, {{"Llega al piso 2", false}, {"Llega al piso 3", false}}, 200, 50});
}

void Game::checkQuestProgress()
{
    if (!player_)
        return;
    int floor = player_->getDungeonFloor();

    for (auto &q : quests_)
    {
        if (q.status == QuestStatus::Completed || q.status == QuestStatus::Failed)
            continue;
        if (q.id == "primer_sangre")
        {
            q.objectives[0].completed = (enemiesKilled_ >= 1);
        }
        else if (q.id == "cazador")
        {
            q.objectives[0].completed = (enemiesKilled_ >= 5);
        }
        else if (q.id == "buscador")
        {
            q.objectives[0].completed = (chestsOpened_ >= 3);
        }
        else if (q.id == "descenso")
        {
            q.objectives[0].completed = (floor >= 2);
            q.objectives[1].completed = (floor >= 3);
        }
        if (q.allObjectivesComplete() && q.status == QuestStatus::InProgress)
        {
            q.status = QuestStatus::Completed;
            player_->gainXp(q.xpReward);
            player_->addCoins(q.goldReward);
            explorationMsg_ = "Mision completada: " + q.title + "!";
        }
    }
}

void Game::render(TerminalScreen &scr)
{
    switch (state_)
    {
    case GameState::MainMenu:
        switch (menuPhase_)
        {
        case MenuPhase::Title:
            Renderer::drawTitle(scr, menuSelection_, hasSave(),
                                std::fmod(GetTime(), 1.0) < 0.5);
            break;
        case MenuPhase::Credits:
            Renderer::drawCredits(scr);
            break;
        case MenuPhase::NameInput:
            Renderer::drawNameInput(scr, playerName_,
                                    std::fmod(GetTime(), 0.8) < 0.4);
            break;
        case MenuPhase::ClassSelect:
            Renderer::drawClassSelect(scr, classSelection_);
            break;
        case MenuPhase::HudSelect:
            Renderer::drawHudSelect(scr, hudSelection_);
            break;
        }
        break;
    case GameState::Exploration:
        if (map_ && player_)
        {
            std::vector<MapEntity> entities;
            for (const auto &we : worldEnemies_)
                if (we.alive)
                    entities.push_back({we.pos, glyphForEnemy(we.type), 6, true});
            for (const auto &ch : worldChests_)
                if (!ch.opened)
                    entities.push_back({ch.pos, '$', 2, true});
            if (lockedDoorExists_ && !lockedDoorOpen_)
                entities.push_back({lockedDoorPos_, '+', 1, false});
            if (!lockedDoorExists_ || lockedDoorOpen_)
                entities.push_back({stairsPos_, '>', 3, true});
            if (shopExists_)
                entities.push_back({shopMerchantPos_, '$', 4, true});
            Renderer::drawExploration(scr, *map_, *player_, hudLayout_,
                                      entities, explorationMsg_);
        }
        break;
    case GameState::Combat:
        if (combat_ && player_)
        {
            bool boss = combatWorldEnemyIdx_ >= 0 && worldEnemies_[combatWorldEnemyIdx_].isBoss;
            int flashIdx = (GetTime() < combatFlashEndTime_) ? combatFlashIdx_ : -1;
            Renderer::drawCombat(scr, *combat_, *player_,
                                 combatShowingArts_, combatArtSelection_, boss, flashIdx);
        }
        break;
    case GameState::Shop:
        if (player_)
            Renderer::drawShop(scr, shopStock_, shopSelection_,
                               *player_, explorationMsg_);
        break;
    case GameState::Inventory:
        if (player_)
            Renderer::drawInventory(scr, *player_, inventorySelection_);
        break;
    case GameState::QuestLog:
        Renderer::drawQuestLog(scr, quests_, questLogSelection_);
        break;
    case GameState::GameOver:
        Renderer::drawGameOver(scr);
        break;
    case GameState::QuitDialog:
        Renderer::drawQuitDialog(scr, menuSelection_);
        break;
    }
}

// ─── helpers ─────────────────────────────────────────────────────────────────

static char glyphForEnemy(EnemyType t)
{
    switch (t)
    {
    case EnemyType::Goblin:
        return 'g';
    case EnemyType::Skeleton:
        return 's';
    case EnemyType::Orc:
        return 'o';
    case EnemyType::Spider:
        return 'a';
    case EnemyType::Vampire:
        return 'V';
    }
    return '?';
}

// Tipo de enemigo según piso con pesos dinámicos
static EnemyType pickEnemyType(int floor)
{
    bool spiderAvail = (floor >= 2);
    bool vampireAvail = (floor >= 4);
    int gobW = std::max(5, 55 - (floor - 1) * 8);
    int skeW = 15;
    int orcW = std::min(30, (floor - 1) * 5);
    int spiW = spiderAvail ? std::min(30, (floor - 2) * 7) : 0;
    int vapW = vampireAvail ? std::min(20, (floor - 4) * 5) : 0;
    int total = gobW + skeW + orcW + spiW + vapW;
    int r = std::rand() % total;
    if (r < gobW)
        return EnemyType::Goblin;
    r -= gobW;
    if (r < skeW)
        return EnemyType::Skeleton;
    r -= skeW;
    if (r < orcW)
        return EnemyType::Orc;
    r -= orcW;
    if (r < spiW)
        return EnemyType::Spider;
    return EnemyType::Vampire;
}

// Stats escalan con el piso: +15% HP/ATK/DEF por piso
static std::unique_ptr<Enemy> makeEnemy(EnemyType t, int floor, bool isBoss)
{
    float s = 1.0f + (floor - 1) * 0.15f;
    auto sc = [s](int base)
    { return std::max(1, static_cast<int>(base * s)); };
    std::string name;
    int hp, atk, def, xp;
    int pa = 1;
    switch (t)
    {
    case EnemyType::Goblin:
        name = "Goblin";
        hp = sc(125);
        atk = sc(17);
        def = sc(4);
        xp = sc(60);
        pa = 1;
        break;
    case EnemyType::Skeleton:
        name = "Esqueleto";
        hp = sc(120);
        atk = sc(16);
        def = sc(8);
        xp = sc(60);
        pa = 1;
        break;
    case EnemyType::Orc:
        name = "Orco";
        hp = sc(140);
        atk = sc(20);
        def = sc(10);
        xp = sc(120);
        pa = 2;
        break;
    case EnemyType::Spider:
        name = "Arana";
        hp = sc(80);
        atk = sc(12);
        def = sc(2);
        xp = sc(40);
        pa = 1;
        break;
    case EnemyType::Vampire:
        name = "Vampiro";
        hp = sc(110);
        atk = sc(18);
        def = sc(6);
        xp = sc(80);
        pa = 1;
        break;
    default:
        name = "???";
        hp = 10;
        atk = 5;
        def = 1;
        xp = 5;
        pa = 1;
        break;
    }
    if (isBoss)
    {
        hp = static_cast<int>(hp * 2.5f);
        atk = static_cast<int>(atk * 2.5f);
        def = static_cast<int>(def * 2.5f);
        switch (t)
        {
        case EnemyType::Goblin:
            name = "Rey Goblin";
            break;
        case EnemyType::Skeleton:
            name = "Senor Liche";
            break;
        case EnemyType::Orc:
            name = "Gran Orco";
            break;
        case EnemyType::Spider:
            name = "Reina Arana";
            break;
        case EnemyType::Vampire:
            name = "Vampiro Anciano";
            break;
        default:
            break;
        }
    }
    return std::make_unique<Enemy>(name, hp, atk, def, xp, t, pa);
}

// XP escala con el piso
static int xpForEnemy(EnemyType t, int floor)
{
    int base;
    switch (t)
    {
    case EnemyType::Goblin:
        base = 20;
        break;
    case EnemyType::Skeleton:
        base = 15;
        break;
    case EnemyType::Orc:
        base = 30;
        break;
    case EnemyType::Spider:
        base = 40;
        break;
    case EnemyType::Vampire:
        base = 80;
        break;
    default:
        base = 5;
        break;
    }
    return base + (floor - 1) * 5;
}

// ─── AI movement ─────────────────────────────────────────────────────────────

// Returns the first step from 'from' toward 'to' avoiding non-walkable tiles.
// Returns 'from' if no path exists or already at destination.
static Position bfsStep(Position from, Position to, const Map &map)
{
    if (from.x == to.x && from.y == to.y)
        return from;
    const int W = map.width(), H = map.height();
    std::vector<int> dist(W * H, -1);
    std::vector<Position> prev(W * H, {-1, -1});
    std::queue<Position> q;
    auto idx = [W](int x, int y)
    { return y * W + x; };
    dist[idx(from.x, from.y)] = 0;
    q.push(from);
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};
    bool found = false;
    while (!q.empty() && !found)
    {
        auto [x, y] = q.front();
        q.pop();
        for (int d = 0; d < 4; d++)
        {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H)
                continue;
            if (dist[idx(nx, ny)] != -1)
                continue;
            if (!map.isWalkable(nx, ny))
                continue;
            dist[idx(nx, ny)] = dist[idx(x, y)] + 1;
            prev[idx(nx, ny)] = {x, y};
            if (nx == to.x && ny == to.y)
            {
                found = true;
                break;
            }
            q.push({nx, ny});
        }
    }
    if (!found)
        return from;
    Position cur = to;
    while (prev[idx(cur.x, cur.y)].x != from.x || prev[idx(cur.x, cur.y)].y != from.y)
    {
        Position p = prev[idx(cur.x, cur.y)];
        if (p.x == -1)
            return from;
        cur = p;
    }
    return cur;
}

void Game::aiLoop()
{
    while (aiRunning_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        if (!aiRunning_)
            break;

        {
            std::lock_guard<std::mutex> lk(worldMutex_);
            if ((state_ != GameState::Exploration && state_ != GameState::Shop) || !map_)
                continue;

            Position player = map_->getPlayerPos();
            bool playerInShop = (state_ == GameState::Shop);

            for (int i = 0; i < static_cast<int>(worldEnemies_.size()); i++)
            {
                auto &we = worldEnemies_[i];
                if (!we.alive)
                    continue;

                // Player in shop → all enemies return to spawn
                if (playerInShop)
                {
                    if (we.pos.x == we.spawnPos.x && we.pos.y == we.spawnPos.y)
                        continue;
                    Position next = bfsStep(we.pos, we.spawnPos, *map_);
                    if (next.x != we.pos.x || next.y != we.pos.y)
                        we.pos = next;
                    continue;
                }

                // Only chase player within Chebyshev distance 10
                int dist = std::max(std::abs(we.pos.x - player.x),
                                    std::abs(we.pos.y - player.y));
                if (dist > 10)
                    continue;

                Position next = bfsStep(we.pos, player, *map_);

                // Safe zone: block entry into shop room
                if (shopExists_ && isInShopRoom(next))
                    continue;

                // Next step lands on player → trigger combat
                if (next.x == player.x && next.y == player.y)
                {
                    if (pendingCombatEnemy_ < 0)
                        pendingCombatEnemy_ = i;
                    continue;
                }

                // Check no other enemy occupies 'next'
                bool occupied = false;
                for (int j = 0; j < static_cast<int>(worldEnemies_.size()); j++)
                {
                    if (j == i || !worldEnemies_[j].alive)
                        continue;
                    if (worldEnemies_[j].pos.x == next.x &&
                        worldEnemies_[j].pos.y == next.y)
                    {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied && (next.x != we.pos.x || next.y != we.pos.y))
                    we.pos = next;
            }
        }

        pendingRedraw_.store(true, std::memory_order_release);
    }
}

// ─── state transitions ────────────────────────────────────────────────────────

void Game::setState(GameState newState)
{
    if (newState == GameState::MainMenu)
    {
        pendingCombatEnemy_ = -1;
        menuPhase_ = MenuPhase::Title;
        menuSelection_ = 0;
        classSelection_ = 0;
        hudSelection_ = 0;
        playerName_.clear();
        worldEnemies_.clear();
        combat_.reset();
    }

    if (newState == GameState::Exploration)
    {
        pendingCombatEnemy_ = -1;
        worldEnemies_.clear();
        worldChests_.clear();
        lockedDoorExists_ = false;
        lockedDoorOpen_ = false;
        combat_.reset();

        map_ = std::make_unique<Map>(80, 40);
        BSPDungeon gen(80, 40);
        gen.generate(*map_);

        const auto &rooms = gen.getRooms();
        int n = static_cast<int>(rooms.size());

        map_->setPlayerPos(rooms[0].centerX(), rooms[0].centerY());
        map_->updateFov();

        // Tracks taken positions to avoid overlaps
        std::vector<Position> taken;
        taken.push_back({rooms[0].centerX(), rooms[0].centerY()});

        // Pick a random free tile inside a room
        auto pickPos = [&](int ri) -> Position
        {
            const auto &r = rooms[ri];
            int iw = std::max(1, r.w - 2);
            int ih = std::max(1, r.h - 2);
            for (int attempt = 0; attempt < 30; attempt++)
            {
                Position p = {r.x + 1 + std::rand() % iw,
                              r.y + 1 + std::rand() % ih};
                bool ok = true;
                for (const auto &t : taken)
                    if (t.x == p.x && t.y == p.y)
                    {
                        ok = false;
                        break;
                    }
                if (ok)
                {
                    taken.push_back(p);
                    return p;
                }
            }
            Position c = {r.centerX(), r.centerY()};
            taken.push_back(c);
            return c;
        };

        // Stairs in last room
        stairsPos_ = pickPos(n - 1);

        // Locked door in second-to-last room (if >= 3 rooms)
        if (n >= 3)
        {
            lockedDoorPos_ = pickPos(n - 2);
            lockedDoorExists_ = true;
        }

        // Shop room: random room that is not rooms[0], rooms[n-1], or rooms[n-2]
        shopExists_ = false;
        shopStock_.clear();
        {
            std::vector<int> shopCandidates;
            for (int i = 1; i < n - 1; i++)
            {
                if (n >= 3 && i == n - 2)
                    continue; // sala de locked door
                shopCandidates.push_back(i);
            }
            if (!shopCandidates.empty())
            {
                int si = shopCandidates[std::rand() % static_cast<int>(shopCandidates.size())];
                shopRoom_ = rooms[si];
                shopMerchantPos_ = {shopRoom_.centerX(), shopRoom_.centerY()};
                shopExists_ = true;
                taken.push_back(shopMerchantPos_);
            }
        }

        // Enemies: spawn chance y tipo escalan con el piso
        int floor = player_ ? player_->getDungeonFloor() : 1;
        int spawnChance = std::min(85, 50 + (floor - 1) * 8); // 50%→85%
        for (int i = 1; i < n; i++)
        {
            if (shopExists_ && rooms[i].x == shopRoom_.x && rooms[i].y == shopRoom_.y)
                continue;
            if (std::rand() % 100 < spawnChance)
            {
                worldEnemies_.push_back({pickPos(i), pickPos(i), pickEnemyType(floor), true});
            }
            // Pisos 3+: 20% extra de tener un segundo enemigo por sala
            if (floor >= 3 && std::rand() % 100 < 20 + floor * 3)
            {
                worldEnemies_.push_back({pickPos(i), pickPos(i), pickEnemyType(floor), true});
            }
        }

        // Chests: 2–4 per floor in random rooms (skip start room and stairs room)
        PlayerClass cls = player_ ? player_->getClass() : PlayerClass::Warrior;
        // floor ya definido arriba
        int chestCount = 2 + std::rand() % 3;

        // Build a shuffled list of eligible rooms (not start, not last)
        std::vector<int> eligible;
        for (int i = 1; i < n - 1; i++)
            eligible.push_back(i);
        for (int i = static_cast<int>(eligible.size()) - 1; i > 0; i--)
            std::swap(eligible[i], eligible[std::rand() % (i + 1)]);

        chestCount = std::min(chestCount, static_cast<int>(eligible.size()));
        bool keyInChest = false;

        for (int ci = 0; ci < chestCount; ci++)
        {
            int roll = std::rand() % 100;
            ChestLoot loot;
            int coins = 0;
            Item item{};

            int baseCoins = 10 + std::rand() % 40;
            if (roll < 50)
            {
                loot = ChestLoot::Coins;
                coins = baseCoins * (1 + (floor - 1) / 2);
            }
            else if (roll < 90)
            {
                loot = ChestLoot::Item;
                int r = std::rand() % 4;
                if (r == 0)
                    item = pickPotion(floor);
                else if (r == 1)
                    item = pickWeapon(cls, floor);
                else
                    item = pickArmor(cls, floor);
            }
            else
            {
                loot = ChestLoot::Key;
                keyInChest = true;
            }

            worldChests_.push_back({pickPos(eligible[ci]), false, loot, coins, item});
        }

        // Guarantee a key source: if no chest has one, force one random chest to carry it
        // (enemies can also drop keys on kill, so this is just a safety net)
        if (lockedDoorExists_ && !keyInChest && !worldChests_.empty())
        {
            int idx = std::rand() % static_cast<int>(worldChests_.size());
            worldChests_[idx] = {worldChests_[idx].pos, false, ChestLoot::Key, 0, {}};
        }

        // Place 1–2 secret rooms hidden behind SecretWall tiles
        int numSecret = 1 + std::rand() % 2;
        for (int i = 0; i < numSecret; i++)
            tryPlaceSecretRoom(taken, cls);

        // Boss cada 5 pisos en el penúltimo cuarto
        if (floor % 5 == 0 && floor > 0 && n >= 3)
        {
            worldEnemies_.push_back({pickPos(n - 2), pickPos(n - 2), pickEnemyType(floor), true, true});
        }
    }

    if (newState == GameState::Combat && player_)
    {
        combatShowingArts_ = false;
        combatArtSelection_ = 0;

        std::vector<std::unique_ptr<Enemy>> enemies;
        int fl = player_ ? player_->getDungeonFloor() : 1;
        if (combatWorldEnemyIdx_ >= 0)
        {
            bool boss = worldEnemies_[combatWorldEnemyIdx_].isBoss;
            enemies.push_back(makeEnemy(worldEnemies_[combatWorldEnemyIdx_].type, fl, boss));
        }
        else
        {
            enemies.push_back(makeEnemy(EnemyType::Goblin, fl));
            enemies.push_back(makeEnemy(EnemyType::Skeleton, fl));
        }
        combat_ = std::make_unique<CombatSystem>(*player_, std::move(enemies));
    }

    state_ = newState;
}

void Game::openChest(WorldChest &chest)
{
    chest.opened = true;
    chestsOpened_++;
    switch (chest.loot)
    {
    case ChestLoot::Coins:
        player_->addCoins(chest.coins);
        explorationMsg_ = "Cofre: +" + std::to_string(chest.coins) + " monedas de oro!";
        break;
    case ChestLoot::Item:
        if (chest.item.type == ItemType::Consumable)
        {
            player_->getInventory().addItem(chest.item);
            explorationMsg_ = "Cofre: encontraste " + chest.item.name + "!";
        }
        else
        {
            player_->applyItemBonus(chest.item);
            explorationMsg_ = "Cofre: encontraste " + chest.item.name + "!  (+" + std::to_string(chest.item.statBonus) + (chest.item.type == ItemType::Weapon ? " ATK" : " DEF") + ")";
        }
        break;
    case ChestLoot::Key:
        player_->addKey();
        explorationMsg_ = "Cofre: encontraste una llave!";
        break;
    }
}

// Items con calidad escalada por piso: +1 statBonus cada 2 pisos
static Item pickWeapon(PlayerClass cls, int floor)
{
    static const std::vector<Item> warrior = {
        {"Espada Corta", "Un filo confiable.", ItemType::Weapon, 30, 1, 3},
        {"Estoque", "Rapido y preciso.", ItemType::Weapon, 30, 1, 3},
        {"Hacha de Mano", "Golpea con fuerza.", ItemType::Weapon, 50, 1, 5},
        {"Espada", "Equilibrada y versatil.", ItemType::Weapon, 50, 1, 5},
        {"Mandoble", "Lenta pero letal.", ItemType::Weapon, 80, 2, 8},
        {"Lanza", "Mantiene la distancia.", ItemType::Weapon, 80, 2, 7},
    };
    static const std::vector<Item> mage = {
        {"Varita de Roble", "Canaliza magia.", ItemType::Weapon, 30, 1, 2},
        {"Varita de Sauco", "Madera antigua y potente.", ItemType::Weapon, 30, 1, 2},
        {"Baculo de Cristal", "Poder arcano.", ItemType::Weapon, 50, 1, 4},
        {"Baculo de Rayos", "Conduce electricidad.", ItemType::Weapon, 50, 1, 4},
        {"Grimorio Oscuro", "Magia devastadora.", ItemType::Weapon, 80, 2, 6},
        {"Grimorio Demoniaco", "Conocimiento prohibido.", ItemType::Weapon, 80, 2, 7},
    };
    static const std::vector<Item> ranger = {
        {"Arco Corto", "Rapido y preciso.", ItemType::Weapon, 30, 1, 3},
        {"Honda", "Simple pero efectiva.", ItemType::Weapon, 30, 1, 2},
        {"Arco Largo", "Mayor alcance.", ItemType::Weapon, 50, 1, 5},
        {"Arco Elfico", "Tallado en madera elfica.", ItemType::Weapon, 50, 1, 5},
        {"Ballesta", "Poderosa y lenta.", ItemType::Weapon, 80, 2, 7},
        {"Arco Encantado", "Flechas magicas.", ItemType::Weapon, 80, 2, 7},
        {"Arco Celestial", "Bendecido por los dioses.", ItemType::Weapon, 80, 2, 8},
    };
    const auto &pool = (cls == PlayerClass::Warrior) ? warrior
                       : (cls == PlayerClass::Mage)  ? mage
                                                     : ranger;
    Item item = pool[std::rand() % pool.size()];
    item.statBonus += (floor - 1) / 2;
    return item;
}

static Item pickArmor(PlayerClass cls, int floor)
{
    static const Item a[3][2] = {
        // Warrior
        {{"Cota de Malla", "Proteccion media.", ItemType::Armor, 40, 1, 3},
         {"Armadura de Placas", "Muy resistente.", ItemType::Armor, 70, 2, 6}},
        // Mage
        {{"Tunica Arcana", "Ligera y magica.", ItemType::Armor, 30, 1, 1},
         {"Manto Mistico", "Deflecta hechizos.", ItemType::Armor, 60, 1, 3}},
        // Ranger
        {{"Cuero Reforzado", "Agil y resistente.", ItemType::Armor, 35, 1, 2},
         {"Armadura de Cuero", "Balance perfecto.", ItemType::Armor, 55, 1, 4}},
    };
    int ci = (cls == PlayerClass::Warrior) ? 0 : (cls == PlayerClass::Mage) ? 1
                                                                            : 2;
    Item item = a[ci][std::rand() % 2];
    item.statBonus += (floor - 1) / 2;
    return item;
}

// Pociones mejoran en pisos altos
static Item pickPotion(int floor)
{
    if (floor >= 5 || std::rand() % 3 == 0)
        return {"Pocion Mayor", "Recupera 80 HP.", ItemType::Consumable, 50, 1, 80};
    if (floor >= 3 || std::rand() % 2 == 0)
        return {"Pocion de Vida", "Recupera 40 HP.", ItemType::Consumable, 25, 1, 40};
    return {"Pocion Pequeña", "Recupera 20 HP.", ItemType::Consumable, 12, 1, 20};
}

void Game::tryPlaceSecretRoom(std::vector<Position> &taken, PlayerClass cls)
{
    // Scan for floor tiles whose adjacent wall could become a secret entrance.
    // Beyond the entrance wall we try to carve a INNER×INNER room in solid wall space.
    static const int DIRS[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    static const int INNER = 3; // 3×3 inner room (total footprint 5×5 with border)

    struct Candidate
    {
        int wallX, wallY, dx, dy;
    };
    std::vector<Candidate> cands;

    for (int y = 2; y < map_->height() - 2; y++)
    {
        for (int x = 2; x < map_->width() - 2; x++)
        {
            if (map_->at(x, y).type != TileType::Floor)
                continue;
            for (auto &d : DIRS)
            {
                int wx = x + d[0], wy = y + d[1];
                if (wx < 1 || wx >= map_->width() - 1 ||
                    wy < 1 || wy >= map_->height() - 1)
                    continue;
                if (map_->at(wx, wy).type == TileType::Wall)
                    cands.push_back({wx, wy, d[0], d[1]});
            }
        }
    }

    // Shuffle candidates
    for (int i = static_cast<int>(cands.size()) - 1; i > 0; i--)
        std::swap(cands[i], cands[std::rand() % (i + 1)]);

    for (const auto &c : cands)
    {
        // Compute the bounding box of the INNER×INNER room interior
        int startX, startY, endX, endY;
        if (c.dx != 0)
        {
            startX = c.wallX + (c.dx > 0 ? 1 : -INNER);
            endX = c.wallX + (c.dx > 0 ? INNER : -1);
            startY = c.wallY - INNER / 2;
            endY = c.wallY + INNER / 2;
        }
        else
        {
            startX = c.wallX - INNER / 2;
            endX = c.wallX + INNER / 2;
            startY = c.wallY + (c.dy > 0 ? 1 : -INNER);
            endY = c.wallY + (c.dy > 0 ? INNER : -1);
        }

        if (startX < 1 || endX >= map_->width() - 1 ||
            startY < 1 || endY >= map_->height() - 1)
            continue;

        // Every tile in the border ring (except the entrance) must be Wall
        bool valid = true;
        for (int by = startY - 1; by <= endY + 1 && valid; by++)
        {
            for (int bx = startX - 1; bx <= endX + 1 && valid; bx++)
            {
                if (bx == c.wallX && by == c.wallY)
                    continue; // entrance
                if (map_->at(bx, by).type != TileType::Wall)
                    valid = false;
            }
        }
        if (!valid)
            continue;

        // Carve the room interior
        for (int y = startY; y <= endY; y++)
            for (int x = startX; x <= endX; x++)
                map_->at(x, y) = {TileType::Floor, '.', false, false};

        // Replace entrance wall with a SecretWall (glyph stays '#')
        map_->at(c.wallX, c.wallY).type = TileType::SecretWall;

        // Place guaranteed good loot: one item chest + coins
        Position itemPos = {startX + 1, startY + 1};
        Position coinsPos = {endX - 1, endY - 1};
        taken.push_back(itemPos);
        int srFloor = player_ ? player_->getDungeonFloor() : 1;
        Item item = (std::rand() % 2 == 0) ? pickWeapon(cls, srFloor) : pickArmor(cls, srFloor);
        worldChests_.push_back({itemPos, false, ChestLoot::Item, 0, item});

        if (coinsPos.x != itemPos.x || coinsPos.y != itemPos.y)
        {
            taken.push_back(coinsPos);
            if (std::rand() % 10 < 3)
                worldChests_.push_back({coinsPos, false, ChestLoot::Item, 0, pickPotion(srFloor)});
            else
            {
                int coins = 20 + std::rand() % 31; // 20–50 coins
                worldChests_.push_back({coinsPos, false, ChestLoot::Coins, coins, {}});
            }
        }

        return; // one secret room is enough per call
    }
}

void Game::returnToExploration()
{
    // Return to the same map without regenerating — just clear combat state
    combat_.reset();
    combatWorldEnemyIdx_ = -1;
    state_ = GameState::Exploration;
}

void Game::inputInventory(int key)
{
    if (!player_)
        return;
    const int bagSize = static_cast<int>(player_->getInventory().items().size());
    const int total = 2 + bagSize;
    switch (key)
    {
    case GKEY_UP:
        inventorySelection_ = (inventorySelection_ - 1 + total) % total;
        break;
    case GKEY_DOWN:
        inventorySelection_ = (inventorySelection_ + 1) % total;
        break;
    case 'e':
    case 'E':
    case '\n':
    {
        int bagIdx = inventorySelection_ - 2;
        if (bagIdx >= 0 && bagIdx < bagSize)
        {
            player_->equipItem(bagIdx);
            int newTotal = 2 + static_cast<int>(player_->getInventory().items().size());
            if (inventorySelection_ >= newTotal)
                inventorySelection_ = std::max(0, newTotal - 1);
        }
        break;
    }
    case 'u':
    case 'U':
    {
        int bagIdx = inventorySelection_ - 2;
        if (bagIdx >= 0 && bagIdx < bagSize)
        {
            const auto &items = player_->getInventory().items();
            if (items[bagIdx].type == ItemType::Consumable)
            {
                std::string name = items[bagIdx].name;
                int bonus = items[bagIdx].statBonus;
                player_->getInventory().removeItem(name);
                int healed = std::min(bonus, player_->getMaxHp() - player_->getHp());
                player_->heal(healed);
                explorationMsg_ = "Usas " + name + ": +" + std::to_string(healed) + " HP!";
                int newTotal = 2 + static_cast<int>(player_->getInventory().items().size());
                if (inventorySelection_ >= newTotal)
                    inventorySelection_ = std::max(0, newTotal - 1);
            }
        }
        break;
    }
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
        break;
    }
}

void Game::inputQuestLog(int key)
{
    int n = static_cast<int>(quests_.size());
    switch (key)
    {
    case GKEY_UP:
        if (n > 0)
            questLogSelection_ = (questLogSelection_ - 1 + n) % n;
        break;
    case GKEY_DOWN:
        if (n > 0)
            questLogSelection_ = (questLogSelection_ + 1) % n;
        break;
    case 27:
    case 'q':
    case 'Q':
        setState(GameState::Exploration);
        break;
    }
}

void Game::inputShop(int key)
{
    int n = static_cast<int>(shopStock_.size());
    if (n == 0)
    {
        state_ = GameState::Exploration;
        return;
    }
    switch (key)
    {
    case GKEY_UP:
        shopSelection_ = (shopSelection_ - 1 + n) % n;
        break;
    case GKEY_DOWN:
        shopSelection_ = (shopSelection_ + 1) % n;
        break;
    case '\n':
    {
        auto &s = shopStock_[shopSelection_];
        if (s.sold)
        {
            explorationMsg_ = "Ya vendido.";
            break;
        }
        if (player_->getCoins() < s.price)
        {
            explorationMsg_ = "No tienes suficiente oro.";
            break;
        }
        player_->addCoins(-s.price);
        if (s.item.type == ItemType::Consumable)
            player_->getInventory().addItem(s.item);
        else
            player_->applyItemBonus(s.item);
        s.sold = true;
        explorationMsg_ = "Compraste: " + s.item.name + "!";
        break;
    }
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
        explorationMsg_.clear();
        break;
    }
}

void Game::generateShopStock()
{
    if (!shopStock_.empty())
        return; // ya generado este piso
    PlayerClass cls = player_->getClass();
    int shopFloor = player_ ? player_->getDungeonFloor() : 1;
    Item p1 = pickPotion(shopFloor);
    Item p2 = pickPotion(shopFloor);
    shopStock_.push_back({p1, p1.value, false});
    shopStock_.push_back({p2, p2.value, false});
    Item w = pickWeapon(cls, shopFloor);
    shopStock_.push_back({w, w.value + shopFloor * 5, false});
    Item a = pickArmor(cls, shopFloor);
    shopStock_.push_back({a, a.value + shopFloor * 5, false});
}

bool Game::isInShopRoom(Position p) const
{
    if (!shopExists_)
        return false;
    return p.x >= shopRoom_.x && p.x < shopRoom_.x + shopRoom_.w &&
           p.y >= shopRoom_.y && p.y < shopRoom_.y + shopRoom_.h;
}

void Game::inputCombat(int key)
{
    if (!combat_)
        return;

    // If combat is over, any key returns to exploration or game over
    if (combat_->isOver())
    {
        if (combat_->playerFled())
        {
            returnToExploration();
        }
        else if (combat_->playerWon())
        {
            if (combatWorldEnemyIdx_ >= 0)
            {
                auto &we = worldEnemies_[combatWorldEnemyIdx_];
                int baseXp = xpForEnemy(we.type, player_->getDungeonFloor());
                int xpGained = we.isBoss ? baseXp * 5 : baseXp;
                player_->gainXp(xpGained);
                we.alive = false;
                enemiesKilled_++;
                int lootChance = we.isBoss ? 45 : 15;
                // chance the enemy drops a key
                if (std::rand() % 100 < lootChance)
                {
                    player_->addKey();
                    explorationMsg_ = "El enemigo solto una llave!";
                }
                // chance the enemy drops a potion
                if (std::rand() % 100 < lootChance)
                {
                    Item pot = pickPotion(player_->getDungeonFloor());
                    player_->getInventory().addItem(pot);
                    explorationMsg_ = "El enemigo solto una " + pot.name + "!";
                }
            }
            // Regeneración post-combate
            int hpRegen = std::max(1, player_->getMaxHp() * 15 / 100);
            int mpRegen = std::max(1, player_->getMaxMana() * 10 / 100);
            player_->heal(hpRegen);
            player_->restoreMana(mpRegen);
            if (explorationMsg_.empty())
                explorationMsg_ = "Recuperas " + std::to_string(hpRegen) + " HP y " + std::to_string(mpRegen) + " MP.";
            returnToExploration();
        }
        else
        {
            setState(GameState::GameOver);
        }
        return;
    }

    if (combatShowingArts_)
    {
        // Arts submenu navigation
        auto arts = player_->getAvailableArts();
        int n = static_cast<int>(arts.size());
        switch (key)
        {
        case GKEY_UP:
            combatArtSelection_ = (combatArtSelection_ - 1 + n) % n;
            break;
        case GKEY_DOWN:
            combatArtSelection_ = (combatArtSelection_ + 1) % n;
            break;
        case '\n':
            combat_->doArt(combatArtSelection_);
            combatFlashIdx_ = combat_->getCurrentTarget();
            combatFlashEndTime_ = GetTime() + 0.25;
            combatShowingArts_ = false;
            break;
        case 27: // ESC
            combatShowingArts_ = false;
            break;
        }
        return;
    }

    // Main action menu
    switch (key)
    {
    case 'a':
    case 'A':
        combat_->doAttack();
        combatFlashIdx_ = combat_->getCurrentTarget();
        combatFlashEndTime_ = GetTime() + 0.25;
        break;
    case 'f':
    case 'F':
        combat_->doHeavyAttack();
        combatFlashIdx_ = combat_->getCurrentTarget();
        combatFlashEndTime_ = GetTime() + 0.25;
        break;
    case 'h':
    case 'H':
        combatShowingArts_ = true;
        combatArtSelection_ = 0;
        break;
    case 'd':
    case 'D':
        combat_->doDefend();
        break;
    case ' ':
        combat_->doEndTurn();
        break;
    case 'r':
    case 'R':
        combat_->doFlee();
        break;
    case 'u':
    case 'U':
        combat_->doUseItem(0);
        break;
    case '\t': // TAB
        combat_->cycleTarget();
        break;
    }
}

// ─── save / load ──────────────────────────────────────────────────────────────

std::string Game::savePath() const
{
    return std::string(GetApplicationDirectory()) + "saves/save.dat";
}

bool Game::hasSave() const
{
    return std::ifstream(savePath()).good();
}

// Write a length-prefixed string: "<len> <chars>"
static void wstr(std::ostream &o, const std::string &s)
{
    o << s.size() << ' ';
    o.write(s.data(), static_cast<std::streamsize>(s.size()));
    o << ' ';
}

// Read a length-prefixed string
static bool rstr(std::istream &in, std::string &s)
{
    size_t n;
    if (!(in >> n))
        return false;
    char sp;
    in.get(sp);
    s.resize(n);
    return n == 0 || in.read(&s[0], static_cast<std::streamsize>(n)).good();
}

static void witem(std::ostream &o, const Item &item)
{
    wstr(o, item.name);
    wstr(o, item.description);
    o << static_cast<int>(item.type) << ' ' << item.value << ' '
      << item.slots << ' ' << item.statBonus << '\n';
}

static bool ritem(std::istream &in, Item &item)
{
    if (!rstr(in, item.name) || !rstr(in, item.description))
        return false;
    int t;
    if (!(in >> t >> item.value >> item.slots >> item.statBonus))
        return false;
    item.type = static_cast<ItemType>(t);
    return true;
}

static char glyphForTile(TileType t)
{
    switch (t)
    {
    case TileType::Wall:
        return '#';
    case TileType::SecretWall:
        return '#';
    case TileType::Floor:
        return '.';
    case TileType::Door:
        return '+';
    case TileType::Trap:
        return '^';
    case TileType::Stairs:
        return '>';
    case TileType::Empty:
        return ' ';
    }
    return ' ';
}

void Game::saveGame()
{
    std::filesystem::create_directories(
        std::string(GetApplicationDirectory()) + "saves");

    std::ofstream f(savePath());
    if (!f || !player_ || !map_)
        return;

    f << 1 << '\n'; // version

    // Player
    wstr(f, player_->name_);
    f << '\n'
      << static_cast<int>(player_->class_) << '\n';
    f << player_->level_ << ' ' << player_->xp_ << ' ' << player_->xpToNextLevel_ << ' '
      << player_->hp_ << ' ' << player_->maxHp_ << ' '
      << player_->mana_ << ' ' << player_->maxMana_ << ' '
      << player_->coins_ << ' ' << player_->keys_ << ' ' << player_->dungeonFloor_ << ' '
      << player_->attack_ << ' ' << player_->defense_ << ' '
      << player_->baseAttack_ << ' ' << player_->baseDefense_ << '\n';

    f << (player_->equippedWeapon_.has_value() ? 1 : 0) << '\n';
    if (player_->equippedWeapon_)
        witem(f, *player_->equippedWeapon_);

    f << (player_->equippedArmor_.has_value() ? 1 : 0) << '\n';
    if (player_->equippedArmor_)
        witem(f, *player_->equippedArmor_);

    const auto &invItems = player_->getInventory().items();
    f << invItems.size() << '\n';
    for (const auto &item : invItems)
        witem(f, item);

    // Map
    f << map_->width() << ' ' << map_->height() << '\n';
    Position pp = map_->getPlayerPos();
    f << pp.x << ' ' << pp.y << '\n';
    for (int y = 0; y < map_->height(); y++)
    {
        for (int x = 0; x < map_->width(); x++)
        {
            const Tile &t = map_->at(x, y);
            f << static_cast<int>(t.type) << ' '
              << static_cast<int>(t.explored) << ' '
              << static_cast<int>(t.visible);
            f << (x < map_->width() - 1 ? ' ' : '\n');
        }
    }

    // World enemies
    f << worldEnemies_.size() << '\n';
    for (const auto &we : worldEnemies_)
    {
        f << we.pos.x << ' ' << we.pos.y << ' '
          << we.spawnPos.x << ' ' << we.spawnPos.y << ' '
          << static_cast<int>(we.type) << ' '
          << static_cast<int>(we.alive) << ' '
          << static_cast<int>(we.isBoss) << '\n';
    }

    // World chests
    f << worldChests_.size() << '\n';
    for (const auto &ch : worldChests_)
    {
        f << ch.pos.x << ' ' << ch.pos.y << ' '
          << static_cast<int>(ch.opened) << ' '
          << static_cast<int>(ch.loot) << ' '
          << ch.coins << ' ';
        witem(f, ch.item);
    }

    // Locked door
    f << static_cast<int>(lockedDoorExists_) << ' ';
    if (lockedDoorExists_)
        f << lockedDoorPos_.x << ' ' << lockedDoorPos_.y << ' '
          << static_cast<int>(lockedDoorOpen_);
    f << '\n';

    // Stairs
    f << stairsPos_.x << ' ' << stairsPos_.y << '\n';

    // Shop
    f << static_cast<int>(shopExists_) << '\n';
    if (shopExists_)
    {
        f << shopMerchantPos_.x << ' ' << shopMerchantPos_.y << ' '
          << shopRoom_.x << ' ' << shopRoom_.y << ' '
          << shopRoom_.w << ' ' << shopRoom_.h << '\n';
    }
    f << shopStock_.size() << '\n';
    for (const auto &s : shopStock_)
    {
        f << static_cast<int>(s.sold) << ' ' << s.price << ' ';
        witem(f, s.item);
    }

    // Quests
    f << enemiesKilled_ << ' ' << chestsOpened_ << '\n';
    f << quests_.size() << '\n';
    for (const auto &q : quests_)
    {
        f << static_cast<int>(q.status) << ' ' << q.objectives.size();
        for (const auto &obj : q.objectives)
            f << ' ' << static_cast<int>(obj.completed);
        f << '\n';
    }

    // HUD layout
    f << static_cast<int>(hudLayout_) << '\n';
}

bool Game::loadGame()
{
    std::ifstream f(savePath());
    if (!f)
        return false;

    int version;
    if (!(f >> version) || version != 1)
        return false;

    // Player
    std::string name;
    if (!rstr(f, name))
        return false;
    int cls;
    if (!(f >> cls))
        return false;

    int level, xp, xpNext, hp, maxHp, mana, maxMana, coins, keys, floor,
        atk, def, baseAtk, baseDef;
    if (!(f >> level >> xp >> xpNext >> hp >> maxHp >> mana >> maxMana >> coins >> keys >> floor >> atk >> def >> baseAtk >> baseDef))
        return false;

    PlayerClass playerClass = static_cast<PlayerClass>(cls);
    player_ = std::make_unique<Player>(name, playerClass);
    player_->level_ = level;
    player_->xp_ = xp;
    player_->xpToNextLevel_ = xpNext;
    player_->hp_ = hp;
    player_->maxHp_ = maxHp;
    player_->mana_ = mana;
    player_->maxMana_ = maxMana;
    player_->coins_ = coins;
    player_->keys_ = keys;
    player_->dungeonFloor_ = floor;
    player_->attack_ = atk;
    player_->defense_ = def;
    player_->baseAttack_ = baseAtk;
    player_->baseDefense_ = baseDef;

    int hasWeapon;
    if (!(f >> hasWeapon))
        return false;
    if (hasWeapon)
    {
        Item w;
        if (!ritem(f, w))
            return false;
        player_->equippedWeapon_ = w;
    }

    int hasArmor;
    if (!(f >> hasArmor))
        return false;
    if (hasArmor)
    {
        Item a;
        if (!ritem(f, a))
            return false;
        player_->equippedArmor_ = a;
    }

    size_t invCount;
    if (!(f >> invCount))
        return false;
    for (size_t i = 0; i < invCount; i++)
    {
        Item item;
        if (!ritem(f, item))
            return false;
        player_->getInventory().addItem(item);
    }

    // Map
    int mapW, mapH, px, py;
    if (!(f >> mapW >> mapH >> px >> py))
        return false;
    map_ = std::make_unique<Map>(mapW, mapH);
    map_->setPlayerPos(px, py);
    for (int y = 0; y < mapH; y++)
    {
        for (int x = 0; x < mapW; x++)
        {
            int type, explored, visible;
            if (!(f >> type >> explored >> visible))
                return false;
            Tile &t = map_->at(x, y);
            t.type = static_cast<TileType>(type);
            t.glyph = glyphForTile(t.type);
            t.explored = explored;
            t.visible = visible;
        }
    }
    map_->updateFov();

    // World enemies
    size_t enemyCount;
    if (!(f >> enemyCount))
        return false;
    worldEnemies_.resize(enemyCount);
    for (auto &we : worldEnemies_)
    {
        int type, alive, boss;
        if (!(f >> we.pos.x >> we.pos.y >> we.spawnPos.x >> we.spawnPos.y >> type >> alive >> boss))
            return false;
        we.type = static_cast<EnemyType>(type);
        we.alive = alive;
        we.isBoss = boss;
    }

    // World chests
    size_t chestCount;
    if (!(f >> chestCount))
        return false;
    worldChests_.resize(chestCount);
    for (auto &ch : worldChests_)
    {
        int opened, loot;
        if (!(f >> ch.pos.x >> ch.pos.y >> opened >> loot >> ch.coins))
            return false;
        ch.opened = opened;
        ch.loot = static_cast<ChestLoot>(loot);
        if (!ritem(f, ch.item))
            return false;
    }

    // Locked door
    int ldExists;
    if (!(f >> ldExists))
        return false;
    lockedDoorExists_ = ldExists;
    lockedDoorOpen_ = false;
    if (lockedDoorExists_)
    {
        int ldOpen;
        if (!(f >> lockedDoorPos_.x >> lockedDoorPos_.y >> ldOpen))
            return false;
        lockedDoorOpen_ = ldOpen;
    }

    // Stairs
    if (!(f >> stairsPos_.x >> stairsPos_.y))
        return false;

    // Shop
    int shopEx;
    if (!(f >> shopEx))
        return false;
    shopExists_ = shopEx;
    if (shopExists_)
    {
        if (!(f >> shopMerchantPos_.x >> shopMerchantPos_.y >> shopRoom_.x >> shopRoom_.y >> shopRoom_.w >> shopRoom_.h))
            return false;
    }
    size_t shopCount;
    if (!(f >> shopCount))
        return false;
    shopStock_.resize(shopCount);
    for (auto &s : shopStock_)
    {
        int sold;
        if (!(f >> sold >> s.price))
            return false;
        s.sold = sold;
        if (!ritem(f, s.item))
            return false;
    }

    // Quests
    if (!(f >> enemiesKilled_ >> chestsOpened_))
        return false;
    size_t questCount;
    if (!(f >> questCount))
        return false;
    initQuests(); // restore titles/descriptions from code
    for (size_t i = 0; i < std::min(questCount, quests_.size()); i++)
    {
        int status, objCount;
        if (!(f >> status >> objCount))
            return false;
        quests_[i].status = static_cast<QuestStatus>(status);
        for (int j = 0; j < objCount && j < static_cast<int>(quests_[i].objectives.size()); j++)
        {
            int completed;
            if (!(f >> completed))
                return false;
            quests_[i].objectives[j].completed = completed;
        }
    }

    // HUD
    int hud;
    if (!(f >> hud))
        return false;
    hudLayout_ = static_cast<HudLayout>(hud);

    // Restore runtime state
    combat_.reset();
    combatWorldEnemyIdx_ = -1;
    pendingCombatEnemy_ = -1;
    explorationMsg_.clear();
    state_ = GameState::Exploration;

    return true;
}
