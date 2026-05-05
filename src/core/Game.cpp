#include "Game.hpp"
#include "GameSerializer.hpp"
#include "core/Assets.hpp"
#include "ai/EnemyAI.hpp"
#include "ui/Renderer.hpp"
#include "entities/Enemy.hpp"
#include "world/DungeonPopulator.hpp"
#include <raylib.h>
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

// Vertical menu navigation: wraps selection_ within [0, n)
static inline void navV(int key, int& sel, int n) {
    if (key == GKEY_UP)   sel = (sel - 1 + n) % n;
    if (key == GKEY_DOWN) sel = (sel + 1) % n;
}

// Forward declarations of file-local helpers (defined near setState)
static char glyphForEnemy(EnemyType t);
static int colorPairForEnemy(EnemyType t);

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
    // Resolución inicial; se permite resize (ver bloque IsWindowResized más abajo)
    const int SCREEN_W = 1280;
    const int SCREEN_H = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "Tenebrarium");
    SetTargetFPS(60);
    SetExitKey(0); // desactivar cierre con ESC (lo manejamos nosotros)
    loadSettings();

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

    static constexpr int kBaseFontSize = 18;

    // Precargamos las tres variantes de tamaño para el zoom del mapa (1×, 2×, 3×).
    // Raylib requiere que los codepoints se declaren al cargar la fuente; de lo
    // contrario los glífos no estarán en la textura y aparecerán como □.
    // TEXTURE_FILTER_POINT evita suavizado que hace borrosos los glífos pixelados.
    auto loadFont = [&](int size) {
        Font f = LoadFontEx((assetsDir() + "fonts/mono.ttf").c_str(), size,
                            codepoints.data(), static_cast<int>(codepoints.size()));
        SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
        return f;
    };
    Font font   = loadFont(kBaseFontSize);
    Font font2x = loadFont(kBaseFontSize * 2);
    Font font3x = loadFont(kBaseFontSize * 3);

    // Celda = ancho de 'M' × (alto de glifo + 2px de margen entre líneas).
    // Se usa 'M' porque es el carácter más ancho en fuentes monoespaciadas.
    Vector2 gs = MeasureTextEx(font, "M", kBaseFontSize, 0);
    int cellW = static_cast<int>(gs.x);
    int cellH = static_cast<int>(gs.y) + 2;

    static constexpr int kPadX = 1;
    static constexpr int kPadY = 1;

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

        int offX = kPadX * cellW;
        int offY = kPadY * cellH;

        int cols = screenW / cellW - 2 * kPadX;
        int rows = screenH / cellH - 2 * kPadY;
        // HudBar: margen extra inferior para que la barra no quede pegada al borde
        if (hudLayout_ == HudLayout::Bottom) rows -= 1;
        if (cols < 1) cols = 1;
        if (rows < 1) rows = 1;

        // Recrear render texture si la ventana cambió de tamaño
        if (IsWindowResized() && (screenW != rtW || screenH != rtH))
        {
            UnloadRenderTexture(renderTarget);
            renderTarget = LoadRenderTexture(screenW, screenH);
            rtW = screenW;
            rtH = screenH;
        }

        // Actualizar uniform de resolución
        float res[2] = {(float)rtW, (float)rtH};
        SetShaderValue(crtShader, resLoc, res, SHADER_UNIFORM_VEC2);

        // El thread de IA escribe pendingCombatEnemy_ y luego activa pendingRedraw_.
        // Aquí consumimos ese evento en el hilo principal para disparar el combate
        // de forma segura (setState no es thread-safe y solo debe llamarse aquí).
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

        TerminalScreen scr(cols, rows, cellW, cellH, font, kBaseFontSize);
        scr.clear();
        render(scr);

        // Pipeline de render de dos pasos:
        // 1) Renderizar la escena completa en la RenderTexture offscreen.
        // 2) Aplicar el shader CRT sobre esa textura al presentar.
        // El overlay de zoom se dibuja sobre la textura offscreen antes de cerrarla.
        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);
        scr.render(offX, offY);

        // Overlay del mapa a font-zoom (solo en exploración y si zoom > 1).
        // Se crea un TerminalScreen adicional con celda más grande y se dibuja
        // encima del área del mapa usando el mismo offset de padding.
        if (mapZoom_ > 1 && state_ == GameState::Exploration && map_ && player_) {
            static constexpr int kSidebarW = 22;
            static constexpr int kHudBarH  = 3;

            int mapPixW, mapPixH;
            if (hudLayout_ == HudLayout::Sidebar) {
                mapPixW = (cols - kSidebarW - 1) * cellW;
                mapPixH = rows * cellH;
            } else {
                mapPixW = cols * cellW;
                mapPixH = (rows - kHudBarH) * cellH;
            }

            // Limpiar el área del mapa con negro para evitar "fantasmas" del zoom x1
            DrawRectangle(offX, offY, mapPixW, mapPixH, BLACK);

            int zCellW = cellW * mapZoom_;
            int zCellH = cellH * mapZoom_;
            Font zFont  = (mapZoom_ == 3) ? font3x : font2x;
            int  zFontH = kBaseFontSize * mapZoom_;
            TerminalScreen mapScr(mapPixW / zCellW, mapPixH / zCellH,
                                  zCellW, zCellH, zFont, zFontH);
            mapScr.clear();

            std::vector<MapEntity> zEntities;
            for (const auto& we : worldEnemies_)
                if (we.alive)
                    zEntities.push_back({we.pos, glyphForEnemy(we.type), colorPairForEnemy(we.type), true});
            for (const auto& ch : worldChests_)
                if (!ch.opened)
                    zEntities.push_back({ch.pos, '$', 2, true});
            if (lockedDoorExists_ && !lockedDoorOpen_)
                zEntities.push_back({lockedDoorPos_, '+', 1, false});
            if (!lockedDoorExists_ || lockedDoorOpen_)
                zEntities.push_back({stairsPos_, '>', 3, true});
            if (shopExists_)
                zEntities.push_back({shopMerchantPos_, '$', 4, true});

            Renderer::drawMap(mapScr, 0, 0, mapScr.cols(), mapScr.rows(),
                               *map_, zEntities);

             mapScr.render(offX, offY);
         }

        EndTextureMode();

        // 2) Presentar con shader CRT.
        // La RenderTexture de Raylib está volteada en Y respecto a OpenGL,
        // por eso el src rect usa altura negativa (flip vertical).
        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(crtShader);
        DrawTexturePro(
            renderTarget.texture,
            {0, 0, (float)rtW, -(float)rtH},
            {0, 0, (float)screenW, (float)screenH},
            {0, 0}, 0.0f, WHITE);
        EndShaderMode();

        // 3) Efecto de explosión de bomba (sobre el tile destruido)
        if (explosionActive_ && GetTime() < explosionEndTime_) {
            // Convertir coordenadas del mapa a píxeles
            Position pp = map_->getPlayerPos();

            int pixelX, pixelY;
            if (mapZoom_ > 1) {
                // Con zoom: usar el mismo cálculo que el mapa con zoom
                int zCellW = cellW * mapZoom_;
                int zCellH = cellH * mapZoom_;
                int mapPixW, mapPixH;
                if (hudLayout_ == HudLayout::Sidebar) {
                    mapPixW = (cols - 22 - 1) * cellW;
                    mapPixH = rows * cellH;
                } else {
                    mapPixW = cols * cellW;
                    mapPixH = (rows - 3) * cellH;
                }
                int camX = pp.x - mapPixW / zCellW / 2;
                int camY = pp.y - mapPixH / zCellH / 2;
                pixelX = offX + (explosionX_ - camX) * zCellW;
                pixelY = offY + (explosionY_ - camY) * zCellH;
            } else {
                // Sin zoom: usar cálculo normal
                int viewW = (hudLayout_ == HudLayout::Sidebar) ? (cols - 22 - 1) : cols;
                int viewH = (hudLayout_ == HudLayout::Bottom) ? (rows - 3) : rows;
                int camX = pp.x - viewW / 2;
                int camY = pp.y - viewH / 2;
                pixelX = offX + (explosionX_ - camX) * cellW;
                pixelY = offY + (explosionY_ - camY) * cellH;
            }
            
            // Determinar tamaño de celda según zoom
            int drawCellW = (mapZoom_ > 1) ? (cellW * mapZoom_) : cellW;
            int drawCellH = (mapZoom_ > 1) ? (cellH * mapZoom_) : cellH;
            
            // Efecto de flash: blanco → naranja → rojo
            float progress = (GetTime() - (explosionEndTime_ - 0.5)) / 0.5f;
            Color expColor;
            if (progress < 0.5f) {
                // Blanco a naranja
                float t = progress * 2.0f;
                expColor = {255, (uint8_t)(255 * (1 - t) + 165 * t), (uint8_t)(255 * (1 - t)), 255};
            } else {
                // Naranja a rojo
                float t = (progress - 0.5f) * 2.0f;
                expColor = {255, (uint8_t)(165 * (1 - t)), 0, 255};
            }

            // Dibujar un rectángulo que cubra el tile completo
            DrawRectangle(pixelX, pixelY, drawCellW, drawCellH, expColor);

            // Dibujar glifos animados
            const char glyphs[] = {'*', '+', '~', '#', '^', 'X'};
            int frame = (int)(GetTime() * 20) % 6;
            int glyphPixelW = (int)MeasureTextEx(font, "X", (float)kBaseFontSize, 0).x;
            int glyphPixelH = drawCellH;
            int glyphX = pixelX + (drawCellW - glyphPixelW) / 2;
            int glyphY = pixelY + (drawCellH - glyphPixelH) / 2;
            DrawTextEx(font, std::string(1, glyphs[frame]).c_str(),
                      {(float)glyphX, (float)glyphY}, (float)kBaseFontSize, 0, BLACK);
        } else {
            explosionActive_ = false;
        }

        // 4) Dibujar mensaje con Raylib directo (encima de todo, sin shader)
        if (!explorationMsg_.empty() && GetTime() < explorationMsgEndTime_) {
            std::string msgText = " " + explorationMsg_ + " ";
            int msgPixelW = (int)MeasureTextEx(font, msgText.c_str(), (float)kBaseFontSize, 0).x;
            int msgPixelH = cellH;

            // Calcular área del mapa según layout
            int mapPixelW, mapPixelH;
            if (hudLayout_ == HudLayout::Sidebar) {
                mapPixelW = (cols - 22 - 1) * cellW;
                mapPixelH = rows * cellH;
            } else {
                mapPixelW = cols * cellW;
                mapPixelH = (rows - 3) * cellH;
            }

            // Centrado horizontalmente respecto al área del mapa
            int pixelX = offX + (mapPixelW - msgPixelW) / 2;
            // 5 filas arriba del borde inferior del mapa
            int pixelY = offY + mapPixelH - (5 * cellH) - msgPixelH;

            Color yellowBg = {255, 230, 60, 255};
            DrawRectangle(pixelX, pixelY, msgPixelW, msgPixelH, yellowBg);

            Color blackFg = {0, 0, 0, 255};
            DrawTextEx(font, msgText.c_str(), {(float)pixelX, (float)pixelY}, (float)kBaseFontSize, 0, blackFg);
        }

        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    UnloadShader(crtShader);
    UnloadFont(font);
    UnloadFont(font2x);
    UnloadFont(font3x);
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
    // Zoom del mapa: numpad primero (antes del loop para capturar 333/334)
    if (IsKeyPressed(334)) { mapZoom_ = std::min(3, mapZoom_ + 1); saveSettings(); return; } // KP_ADD
    if (IsKeyPressed(333)) { mapZoom_ = std::max(1, mapZoom_ - 1); saveSettings(); return; } // KP_SUBTRACT

    for (auto &m : mapping)
    {
        if (IsKeyPressed(m.rl))
        {
            dispatchInput(m.g);
            return;
        }
    }

    // Carácter Unicode — interceptar + y - para zoom antes de dispatch
    int cp = GetCharPressed();
    if (cp == '+') { mapZoom_ = std::min(3, mapZoom_ + 1); saveSettings(); return; }
    if (cp == '-') { mapZoom_ = std::max(1, mapZoom_ - 1); saveSettings(); return; }
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
        // No limpiar mensaje aquí, se oculta solo tras 2 segundos
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
            explorationMsgEndTime_ = GetTime() + 2.0;
            break;
        }

        // Open inventory
        if (key == 'i' || key == 'I')
        {
            inventorySelection_ = 0;
            inventoryTab_ = 0;
            inventoryMsg_.clear();
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

        // Search adjacent tiles for secret walls / use bomb
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
                    // Try to use bomb first, if none available reveal normally
                    int bombCount = 0;
                    for (const auto& item : player_->getInventory().items())
                        if (item.type == ItemType::Bomb) bombCount += item.quantity;
                    if (bombCount > 0) {
                        map_->destroyTile(ax, ay);
                        player_->getInventory().removeItem("Bomba");
                        // Activar efecto visual de explosión
                        explosionActive_ = true;
                        explosionEndTime_ = GetTime() + 0.5;
                        explosionX_ = ax;
                        explosionY_ = ay;
                        map_->updateFov();
                        explorationMsg_ = "BOOM! La pared se hace polvo!";
                    } else {
                        map_->revealSecretWall(ax, ay);
                        map_->updateFov();
                        explorationMsg_ = "Encontraste una sala secreta!";
                    }
                    explorationMsgEndTime_ = GetTime() + 2.0;
                    found = true;
                    break;
                }
            }
            if (!found)
                explorationMsg_ = "No hay nada aqui...";
            explorationMsgEndTime_ = GetTime() + 2.0;
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
            explorationMsg_ = "La puerta permanece sellada. Algo en las sombras aun respira.";
            explorationMsgEndTime_ = GetTime() + 2.0;
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
            if (player_->getDungeonFloor() >= 20) {
                victory_ = true;
                setState(GameState::GameOver);
            } else {
                player_->descendFloor();
                setState(GameState::Exploration);
                explorationMsg_ = "Desciendes al piso " +
                                  std::to_string(player_->getDungeonFloor()) + "...";
                explorationMsgEndTime_ = GetTime() + 2.0;
                saveGame();
            }
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
        if (key == '\n' || key == 27)
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
    switch (key)
    {
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
    case 27: // ESC — cancelar
        returnToExploration();
        break;
    }
}

void Game::inputNameInput(int key)
{
    if (key == '\n')
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
    case GKEY_DOWN:
        navV(key, classSelection_, 3);
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
        // Dar 5 bombas iniciales (como 1 item con quantity=5)
        {
            Item bomba = DungeonPopulator::pickBomb(1);
            bomba.quantity = 5;
            player_->getInventory().addItem(bomba);
        }
        initQuests();
        setState(GameState::Exploration);
        break;
    }
    }
}

void Game::update()
{
    checkQuestProgress();

    // Abrir puerta bloqueada cuando todos los enemigos del piso estén muertos
    if (state_ == GameState::Exploration &&
        lockedDoorExists_ && !lockedDoorOpen_)
    {
        bool anyAlive = false;
        for (const auto &we : worldEnemies_)
            if (we.alive)
            {
                anyAlive = true;
                break;
            }

        if (!anyAlive)
        {
            lockedDoorOpen_ = true;
            explorationMsg_ = "El silencio se apodera del Tenebrarium... algo cede en la oscuridad.";
            explorationMsgEndTime_ = GetTime() + 2.0;
        }
    }
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
            explorationMsgEndTime_ = GetTime() + 2.0;
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
                    entities.push_back({we.pos, glyphForEnemy(we.type), colorPairForEnemy(we.type), true});
            for (const auto &ch : worldChests_)
                if (!ch.opened)
                    entities.push_back({ch.pos, '$', 2, true});
            if (lockedDoorExists_ && !lockedDoorOpen_)
                entities.push_back({lockedDoorPos_, '+', 1, false});
            if (!lockedDoorExists_ || lockedDoorOpen_)
                entities.push_back({stairsPos_, '>', 3, true});
            if (shopExists_)
                entities.push_back({shopMerchantPos_, '$', 4, true});
            // No pasar mensaje (se dibuja aparte con Raylib directo)
            Renderer::drawExploration(scr, *map_, *player_, hudLayout_,
                                       entities, "", mapZoom_);
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
            Renderer::drawInventory(scr, *player_, inventorySelection_, inventoryTab_, inventoryMsg_);
        break;
    case GameState::QuestLog:
        Renderer::drawQuestLog(scr, quests_, questLogSelection_);
        break;
    case GameState::GameOver:
        Renderer::drawGameOver(scr, victory_);
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
    case EnemyType::Goblin:   return 'g';
    case EnemyType::Skeleton: return 's';
    case EnemyType::Orc:      return 'o';
    case EnemyType::Spider:   return 'a';
    case EnemyType::Vampire:  return 'V';
    case EnemyType::Zombie:   return 'z';
    case EnemyType::Demon:    return 'd';
    case EnemyType::Shadow:   return 'S';
    }
    return '?';
}

static int colorPairForEnemy(EnemyType t)
{
    switch (t)
    {
    case EnemyType::Goblin:   return 10; // COL_DK_GREEN
    case EnemyType::Skeleton: return  7; // COL_GRAY
    case EnemyType::Orc:      return  9; // COL_ORANGE
    case EnemyType::Spider:   return  8; // COL_MAGENTA
    case EnemyType::Vampire:  return  6; // COL_RED
    case EnemyType::Zombie:   return 10; // COL_DK_GREEN
    case EnemyType::Demon:    return  6; // COL_RED
    case EnemyType::Shadow:   return 11; // COL_DK_GRAY
    }
    return 6; // COL_RED fallback
}

// ─── AI movement ─────────────────────────────────────────────────────────────

void Game::aiLoop() { EnemyAI::run(*this); }

// ─── state transitions ────────────────────────────────────────────────────────

// setState() es el único lugar donde se crean/destruyen el mapa, los enemigos
// y el sistema de combate. Centralizar aquí evita estados inconsistentes.
// NOTA: returnToExploration() NO pasa por aquí — vuelve al mapa existente.
void Game::setState(GameState newState)
{
    if (newState == GameState::MainMenu)
    {
        // Guardar antes de limpiar (permitir continuar después)
        if (player_ && map_)
            saveGame();
        pendingCombatEnemy_ = -1;
        victory_ = false;
        menuPhase_ = MenuPhase::Title;
        menuSelection_ = 0;
        classSelection_ = 0;
        hudSelection_ = 0;
        playerName_.clear();
        player_.reset(); // Limpiar jugador (incluye inventario)
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
        shopStock_.clear();  // el stock se regenera la primera vez que se entra a la tienda del nuevo piso

        // Generar nuevo piso: mapa BSP 80×40 con jugador en el centro de la sala 0
        map_ = std::make_unique<Map>(80, 40);
        BSPDungeon gen(80, 40);
        gen.generate(*map_);

        map_->setPlayerPos(gen.getRooms()[0].centerX(), gen.getRooms()[0].centerY());
        map_->updateFov();

        int floor = player_ ? player_->getDungeonFloor() : 1;
        PlayerClass cls = player_ ? player_->getClass() : PlayerClass::Warrior;

        auto pop = DungeonPopulator::populate(*map_, gen.getRooms(), floor, cls);
        worldEnemies_    = std::move(pop.enemies);
        worldChests_     = std::move(pop.chests);
        stairsPos_       = pop.stairsPos;
        lockedDoorPos_   = pop.lockedDoorPos;
        lockedDoorExists_= pop.lockedDoorExists;
        shopExists_      = pop.shopExists;
        shopRoom_        = pop.shopRoom;
        shopMerchantPos_ = pop.shopMerchantPos;
    }

    if (newState == GameState::Combat && player_)
    {
        combatShowingArts_ = false;
        combatArtSelection_ = 0;

        // Instanciar el Enemy de combate a partir del WorldEnemy del mundo.
        // Si combatWorldEnemyIdx_ == -1 (caso raro/debug) se usan dos enemigos por defecto.
        std::vector<std::unique_ptr<Enemy>> enemies;
        int fl = player_ ? player_->getDungeonFloor() : 1;
        if (combatWorldEnemyIdx_ >= 0)
        {
            const auto& we = worldEnemies_[combatWorldEnemyIdx_];
            auto enemy = DungeonPopulator::makeEnemy(we.type, fl, we.isBoss);
            if (we.remainingHp > 0 && we.remainingHp < enemy->getMaxHp())
                enemy->takeDamageRaw(enemy->getMaxHp() - we.remainingHp);
            enemies.push_back(std::move(enemy));
        }
        else
        {
            enemies.push_back(DungeonPopulator::makeEnemy(EnemyType::Goblin, fl));
            enemies.push_back(DungeonPopulator::makeEnemy(EnemyType::Skeleton, fl));
        }
        combat_ = std::make_unique<CombatSystem>(*player_, std::move(enemies));
    }

    if (newState == GameState::GameOver)
        GameSerializer::deleteSave();

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
        explorationMsgEndTime_ = GetTime() + 2.0;
        break;
    case ChestLoot::Item:
        if (chest.item.type == ItemType::Consumable)
        {
            player_->getInventory().addItem(chest.item);
            explorationMsg_ = "Cofre: encontraste " + chest.item.name + "!";
        }
        else
        {
            player_->pickupItem(chest.item);
            explorationMsg_ = "Cofre: encontraste " + chest.item.name + "!  (+" + std::to_string(chest.item.statBonus) + (chest.item.type == ItemType::Weapon ? " ATK" : " DEF") + ")";
        }
        explorationMsgEndTime_ = GetTime() + 2.0;
        break;
    }
}


void Game::returnToExploration()
{
    // Return to the same map without regenerating — just clear combat state
    combat_.reset();
    combatWorldEnemyIdx_ = -1;
    explorationMsg_.clear(); // Limpiar mensaje anterior al volver
    state_ = GameState::Exploration;
}

void Game::inputInventory(int key)
{
    if (!player_) return;

    const auto& allItems = player_->getInventory().items();

    // Construir vista filtrada+ordenada igual que el renderer
    auto buildTab = [&](int tab) {
        std::vector<int> idx;
        for (int i = 0; i < static_cast<int>(allItems.size()); i++) {
            bool isConsumable = (allItems[i].type == ItemType::Consumable);
            if (tab == 0 ? isConsumable : !isConsumable)
                idx.push_back(i);
        }
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return allItems[a].statBonus > allItems[b].statBonus;
        });
        return idx;
    };

    // Cambiar pestaña con ←/→
    if (key == GKEY_LEFT || key == GKEY_RIGHT) {
        inventoryTab_ = 1 - inventoryTab_;
        inventorySelection_ = 0;
        inventoryMsg_.clear();
        return;
    }

    if (key == 27 || key == 'q' || key == 'Q') {
        state_ = GameState::Exploration;
        return;
    }

    if (inventoryTab_ == 0) {
        // ── Pestaña Pociones ──────────────────────────────────────────────────
        auto idx = buildTab(0);
        int n = static_cast<int>(idx.size());
        if (key == GKEY_UP || key == GKEY_DOWN) {
            if (n > 0) { navV(key, inventorySelection_, n); inventoryMsg_.clear(); }
            return;
        }
        if ((key == 'e' || key == 'E' || key == 'u' || key == 'U' || key == '\n') && n > 0) {
            int bagIdx = idx[inventorySelection_];
            const Item& item = allItems[bagIdx];
            if (player_->getHp() >= player_->getMaxHp()) {
                inventoryMsg_ = "HP ya esta al maximo!";
            } else {
                std::string name = item.name;
                int bonus = item.statBonus;
                player_->getInventory().removeItem(name);
                int healed = std::min(bonus, player_->getMaxHp() - player_->getHp());
                player_->heal(healed);
                explorationMsg_ = "Usas " + name + ": +" + std::to_string(healed) + " HP!";
                explorationMsgEndTime_ = GetTime() + 2.0;
                int newTotal = 2 + static_cast<int>(player_->getInventory().items().size());
                if (inventorySelection_ >= newTotal)
                    inventorySelection_ = std::max(0, newTotal - 1);
            }
        }
    }
}

void Game::inputQuestLog(int key)
{
    int n = static_cast<int>(quests_.size());
    switch (key)
    {
    case GKEY_UP:
    case GKEY_DOWN:
        if (n > 0) navV(key, questLogSelection_, n);
        break;
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
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
    case GKEY_DOWN:
        navV(key, shopSelection_, n);
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
            explorationMsgEndTime_ = GetTime() + 2.0;
            break;
        }
        player_->addCoins(-s.price);
        if (s.item.type == ItemType::Consumable || s.item.type == ItemType::Bomb)
            player_->getInventory().addItem(s.item);
        else
            player_->pickupItem(s.item);
        s.sold = true;
        explorationMsg_ = "Compraste: " + s.item.name + "!";
        explorationMsgEndTime_ = GetTime() + 2.0;
        break;
    }
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
        // No limpiar mensaje, se oculta solo tras 2 segundos
        break;
    }
}

void Game::generateShopStock()
{
    // Stock lazy: se genera la primera vez que el jugador entra a la tienda del piso.
    // shopStock_.clear() se llama en setState(Exploration) al cambiar de piso.
    if (!shopStock_.empty())
        return;
    PlayerClass cls = player_->getClass();
    int shopFloor = player_ ? player_->getDungeonFloor() : 1;
    Item p1 = DungeonPopulator::pickPotion(shopFloor);
    Item p2 = DungeonPopulator::pickPotion(shopFloor);
    shopStock_.push_back({p1, p1.value, false});
    shopStock_.push_back({p2, p2.value, false});
    Item bomb = DungeonPopulator::pickBomb(shopFloor);
    bomb.quantity = 3; // Vender en grupos de 3
    shopStock_.push_back({bomb, bomb.value, false});
    Item w = DungeonPopulator::pickWeapon(cls, shopFloor);
    shopStock_.push_back({w, w.value + shopFloor * 5, false});
    Item a = DungeonPopulator::pickArmor(cls, shopFloor);
    shopStock_.push_back({a, a.value + shopFloor * 5, false});
}

bool Game::isInShopRoom(Position p) const
{
    if (!shopExists_)
        return false;
    return p.x >= shopRoom_.x && p.x < shopRoom_.x + shopRoom_.w &&
           p.y >= shopRoom_.y && p.y < shopRoom_.y + shopRoom_.h;
}

// Usar bomba en pared secreta adyacente (tecla E)
void Game::useBomb()
{
    if (!player_ || !map_) return;

    // Verificar si tiene bombas (sumar quantity total)
    int bombCount = 0;
    for (const auto& item : player_->getInventory().items())
        if (item.type == ItemType::Bomb) bombCount += item.quantity;

    if (bombCount == 0) {
        explorationMsg_ = "No tienes bombas.";
        explorationMsgEndTime_ = GetTime() + 2.0;
        return;
    }

    Position pos = map_->getPlayerPos();
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    bool destroyed = false;

    // Buscar pared secreta adyacente
    int bombX = 0, bombY = 0;
    for (int i = 0; i < 4; i++) {
        int ax = pos.x + dx[i], ay = pos.y + dy[i];
        if (map_->isSecretWall(ax, ay)) {
            map_->destroyTile(ax, ay);
            bombX = ax;
            bombY = ay;
            destroyed = true;
            break;
        }
    }

    if (!destroyed) {
        explorationMsg_ = "No hay pared secreta adyacente.";
        explorationMsgEndTime_ = GetTime() + 2.0;
        return;
    }

    // Consumir bomba y activar efecto visual (0.5s)
    player_->getInventory().removeItem("Bomba");
    explosionActive_ = true;
    explosionEndTime_ = GetTime() + 0.5;
    explosionX_ = bombX;
    explosionY_ = bombY;
    explorationMsg_ = "BOOM! La pared se hace polvo!";
    explorationMsgEndTime_ = GetTime() + 2.0;
    map_->updateFov(); // Actualizar visibilidad
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
            if (combatWorldEnemyIdx_ >= 0 && !combat_->getEnemies().empty())
                worldEnemies_[combatWorldEnemyIdx_].remainingHp =
                    combat_->getEnemies()[0]->getHp();
            returnToExploration();
        }
        else if (combat_->playerWon())
        {
            if (combatWorldEnemyIdx_ >= 0)
            {
                auto &we = worldEnemies_[combatWorldEnemyIdx_];
                int baseXp = DungeonPopulator::xpForEnemy(we.type, player_->getDungeonFloor());
                int xpGained = we.isBoss ? baseXp * 5 : baseXp;
                player_->gainXp(xpGained);
                we.alive = false;
                enemiesKilled_++;
                int lootChance = we.isBoss ? 45 : 15;
                // chance the enemy drops a potion
                if (std::rand() % 100 < lootChance)
                {
                    Item pot = DungeonPopulator::pickPotion(player_->getDungeonFloor());
                    player_->getInventory().addItem(pot);
                    explorationMsg_ = "El enemigo solto una " + pot.name + "!";
                    explorationMsgEndTime_ = GetTime() + 2.0;
                }
            }
            // Regeneración post-combate
            int hpRegen = std::max(1, player_->getMaxHp() * 15 / 100);
            int mpRegen = std::max(1, player_->getMaxMana() * 10 / 100);
            player_->heal(hpRegen);
            player_->restoreMana(mpRegen);
            if (explorationMsg_.empty())
                explorationMsg_ = "Recuperas " + std::to_string(hpRegen) + " HP y " + std::to_string(mpRegen) + " MP.";
            explorationMsgEndTime_ = GetTime() + 2.0;
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
        case GKEY_DOWN:
            navV(key, combatArtSelection_, n);
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
        combat_->doUseItem();
        break;
    case '\t': // TAB
        combat_->cycleTarget();
        break;
    }
}

// ─── save / load ──────────────────────────────────────────────────────────────

std::string Game::savePath() const { return GameSerializer::savePath(); }
bool        Game::hasSave()  const { return GameSerializer::hasSave();  }

void Game::saveGame() { GameSerializer::save(*this); }
bool Game::loadGame() { return GameSerializer::load(*this); }

void Game::saveSettings() const {
    std::string dir = std::string(GetApplicationDirectory()) + "saves";
    std::filesystem::create_directories(dir);
    std::ofstream f(dir + "/settings.dat");
    if (f) f << "mapZoom=" << mapZoom_ << "\n";
}

void Game::loadSettings() {
    std::ifstream f(std::string(GetApplicationDirectory()) + "saves/settings.dat");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("mapZoom=", 0) == 0) {
            try { mapZoom_ = std::clamp(std::stoi(line.substr(8)), 1, 3); }
            catch (...) {}
        }
    }
}
