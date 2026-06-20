#include "Game.hpp"
#include "GameSerializer.hpp"
#include "core/Assets.hpp"
#include "ai/EnemyAI.hpp"
#include "ui/Renderer.hpp"
#include "entities/Enemy.hpp"
#include "world/DungeonPopulator.hpp"
#include <raylib.h>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <unordered_map>

// Keycode interno del juego (evita conflicto con KeyboardKey de Raylib)
static constexpr int GKEY_BACKSPACE = 0x1005;

// Vertical menu navigation: wraps selection_ within [0, n)
static inline void navV(int key, int& sel, int n) {
    if (key == 'w') sel = (sel - 1 + n) % n;
    if (key == 's') sel = (sel + 1) % n;
}

// Forward declarations of file-local helpers (defined near setState)
static int glyphForEnemy(EnemyType t);
static int colorPairForEnemy(EnemyType t);

Game::Game()
    : menuPhase_(MenuPhase::Title),
      quitRequested_(false),
      menuSelection_(0),
      classSelection_(0),
      hudSelection_(0),
      hudLayout_(HudLayout::Sidebar),
      combatShowingArts_(false),
      combatArtSelection_(0),
      combatFlashIdx_(-1),
      combatFlashEndTime_(0.0)
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
    SetWindowMinSize(900, 500);
    SetTargetFPS(60);
    SetExitKey(0); // desactivar cierre con ESC (lo manejamos nosotros)
    loadSettings();

    // Ruta de assets relativa al ejecutable (funciona sin importar dónde esté el binario)
    assetsDir() = std::string(GetApplicationDirectory()) + "assets/";

    // Limpiar capturas viejas de Raylib que hayan quedado de ejecuciones anteriores
    for (int i = 0; i < 1000; i++) {
        char p[64];
        snprintf(p, sizeof(p), "screenshot%03i.png", i);
        if (!std::filesystem::exists(p)) break;
        std::filesystem::remove(p);
    }
    screenshotCounter_ = 0;

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
        0x00C1, // Á
        0x00CD, // Í
        0x00D3, // Ó
        0x00DA, // Ú
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

    static constexpr int kRefFontSize = 18;
    static constexpr int kTargetCols  = 130;
    static constexpr int kTargetRows  = 48;

    auto loadFont = [&](int size) {
        Font f = LoadFontEx((assetsDir() + "fonts/mono.ttf").c_str(), size,
                            codepoints.data(), static_cast<int>(codepoints.size()));
        SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
        return f;
    };

    Font refFont = loadFont(kRefFontSize);
    Vector2 refGs = MeasureTextEx(refFont, "M", (float)kRefFontSize, 0);
    float refCellW       = refGs.x;
    float refCellH       = refGs.y;
    float cellWperFontPt = refCellW / (float)kRefFontSize;
    float cellHperFontPt = refCellH / (float)kRefFontSize;

    std::unordered_map<int, Font> fontCache;
    fontCache[kRefFontSize] = refFont;
    auto getFont = [&](int size) -> Font& {
        auto it = fontCache.find(size);
        if (it != fontCache.end()) return it->second;
        Font f = loadFont(size);
        fontCache[size] = f;
        return fontCache[size];
    };

    static constexpr int kPadX = 1;
    static constexpr int kPadY = 1;

    // Shader CRT
    Shader crtShader = LoadShader(0, (assetsDir() + "shaders/crt.frag").c_str());
    int resLoc = GetShaderLocation(crtShader, "resolution");

    // Render texture offscreen
    RenderTexture2D renderTarget = LoadRenderTexture(SCREEN_W, SCREEN_H);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_POINT);
    int rtW = SCREEN_W, rtH = SCREEN_H;

    while (!WindowShouldClose() && !quitRequested_)
    {
        // Recalcular grid si la ventana cambió de tamaño
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        // Escalar fontSize para mantener ~kTargetCols × kTargetRows
        float cellWideal = (float)screenW / (kTargetCols + 2 * kPadX);
        float cellHideal = (float)screenH / (kTargetRows + 2 * kPadY);
        int fontSizeFromW = (int)(cellWideal / cellWperFontPt);
        int fontSizeFromH = (int)(cellHideal / cellHperFontPt);
        int fontSize = std::clamp(std::min(fontSizeFromW, fontSizeFromH), 10, kRefFontSize);
        Font& font   = getFont(fontSize);
        Font& font2x = getFont(fontSize * 2);
        Font& font3x = getFont(fontSize * 3);

        Vector2 gs = MeasureTextEx(font, "M", (float)fontSize, 0);
        int cellW = static_cast<int>(gs.x);
        int cellH = static_cast<int>(gs.y);

        int offX = kPadX * cellW;
        int offY = kPadY * cellH;

        int cols = screenW / cellW - 2 * kPadX;
        int rows = screenH / cellH - 2 * kPadY;
        // HudBar: margen extra inferior para que la barra no quede pegada al borde
        if (hudLayout_ == HudLayout::Bottom) rows -= 1;
        constexpr int kSidebarW = 30;
        constexpr int kHudBarH  = 13;
        if (cols < 1) cols = 1;
        if (rows < 1) rows = 1;

        // Recrear render texture si la ventana cambió de tamaño
        if (IsWindowResized() && (screenW != rtW || screenH != rtH))
        {
            UnloadRenderTexture(renderTarget);
            renderTarget = LoadRenderTexture(screenW, screenH);
            SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_POINT);
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
            int combatIdx = pendingCombatEnemy_.load();
            if (combatIdx >= 0 && state_.load() == GameState::Exploration)
            {
                combatWorldEnemyIdx_ = combatIdx;
                pendingCombatEnemy_.store(-1);
                setState(GameState::Combat);
            }
        }

        processInput();
        update();

        TerminalScreen scr(cols, rows, cellW, cellH, font, fontSize);
        scr.clear();
        render(scr);

        // Pipeline de render de dos pasos:
        // 1) Renderizar la escena completa en la RenderTexture offscreen.
        // 2) Aplicar el shader CRT sobre esa textura al presentar.
        // El overlay de zoom se dibuja sobre la textura offscreen antes de cerrarla.
        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);
        scr.render(offX, offY);

        if (mapZoom_ > 1 && state_.load() == GameState::Exploration && dungeon_ && player_) {

            int mapPixW, mapPixH;
            if (hudLayout_ == HudLayout::Sidebar) {
                mapPixW = (cols - kSidebarW - 1) * cellW;
                mapPixH = rows * cellH;
            } else {
                mapPixW = cols * cellW;
                mapPixH = (rows - kHudBarH) * cellH;
            }

            // Limpiar solo el interior del mapa, dejando el borde intacto
            DrawRectangle(offX + cellW, offY + cellH,
                          mapPixW - 2 * cellW, mapPixH - 2 * cellH, BLACK);

            int zCellW = cellW * mapZoom_;
            int zCellH = cellH * mapZoom_;
            Font zFont  = (mapZoom_ == 3) ? font3x : font2x;
            int  zFontH = fontSize * mapZoom_;
            int innerPixW = mapPixW - 2 * cellW;
            int innerPixH = mapPixH - 2 * cellH;
            TerminalScreen mapScr(innerPixW / zCellW, innerPixH / zCellH,
                                  zCellW, zCellH, zFont, zFontH);
            mapScr.clear();

            std::vector<MapEntity> zEntities;
            {
                auto acc = dungeon_->lock();
                for (const auto& we : acc.enemies())
                    if (we.alive)
                        zEntities.push_back({we.pos, glyphForEnemy(we.type), colorPairForEnemy(we.type), true});
                for (const auto& ch : acc.chests())
                    if (!ch.opened)
                        zEntities.push_back({ch.pos, '$', 2, true});
                if (acc.lockedDoorExists() && !acc.lockedDoorOpen())
                    zEntities.push_back({acc.lockedDoorPos(), '+', 1, false});
                if (!acc.lockedDoorExists() || acc.lockedDoorOpen())
                    zEntities.push_back({acc.stairsPos(), '>', 3, true, true});
                if (acc.shopExists())
                    zEntities.push_back({acc.shopMerchantPos(), '$', 4, true});

                Renderer::drawMap(mapScr, 0, 0,
                                   mapScr.cols(), mapScr.rows(),
                                   acc.map(), zEntities,
                                   Renderer::colorForPlayerClass(player_->getClass()),
                                   player_->getDungeonFloor());
            }

             mapScr.render(offX + cellW, offY + cellH);
         }

        EndTextureMode();

    // Shake de la pala (desplaza la textura renderizada)
    float shakeX = 0, shakeY = 0;
    if (shovelDigging_ && GetTime() < shovelDigEndTime_) {
        float progress = (shovelDigEndTime_ - GetTime()) / 0.6f;
        float intensity = 3.0f * progress;
        shakeX = (float)((std::rand() % 7) - 3) * intensity / 3.0f;
        shakeY = (float)((std::rand() % 7) - 3) * intensity / 3.0f;
    } else if (shovelDigging_) {
        shovelDigging_ = false;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    if (shaderEnabled_) BeginShaderMode(crtShader);
    DrawTexturePro(
        renderTarget.texture,
        {0, 0, (float)rtW, -(float)rtH},
        {shakeX, shakeY, (float)screenW, (float)screenH},
        {0, 0}, 0.0f, WHITE);
    if (shaderEnabled_) EndShaderMode();

        // 3) Efecto de explosión de bomba (sobre el tile destruido)
        if (dungeon_ && dungeon_->explosionActive && GetTime() < dungeon_->explosionEndTime) {
            // Convertir coordenadas del mapa a píxeles
            Position pp = dungeon_->lock().playerPos();

            int pixelX, pixelY;
            if (mapZoom_ > 1) {
                // Con zoom: usar el mismo cálculo que el mapa con zoom
                int zCellW = cellW * mapZoom_;
                int zCellH = cellH * mapZoom_;
                int mapPixW, mapPixH;
                if (hudLayout_ == HudLayout::Sidebar) {
                    mapPixW = (cols - kSidebarW - 1) * cellW;
                    mapPixH = rows * cellH;
                } else {
                    mapPixW = cols * cellW;
                    mapPixH = (rows - kHudBarH) * cellH;
                }
                int zoomCols = (mapPixW - 2 * cellW) / zCellW;
                int zoomRows = (mapPixH - 2 * cellH) / zCellH;
                int camX = pp.x - zoomCols / 2;
                int camY = pp.y - zoomRows / 2;
                pixelX = offX + cellW + (dungeon_->explosionX - camX) * zCellW;
                pixelY = offY + cellH + (dungeon_->explosionY - camY) * zCellH;
            } else {
                // Sin zoom: usar cálculo normal
                int viewW = (hudLayout_ == HudLayout::Sidebar) ? (cols - kSidebarW - 1) : cols;
                int viewH = (hudLayout_ == HudLayout::Bottom) ? (rows - kHudBarH) : rows;
                int camX = pp.x - viewW / 2;
                int camY = pp.y - viewH / 2;
                pixelX = offX + (dungeon_->explosionX - camX) * cellW;
                pixelY = offY + (dungeon_->explosionY - camY) * cellH;
            }
            
            // Determinar tamaño de celda según zoom
            int drawCellW = (mapZoom_ > 1) ? (cellW * mapZoom_) : cellW;
            int drawCellH = (mapZoom_ > 1) ? (cellH * mapZoom_) : cellH;
            
            // Efecto de flash: blanco → naranja → rojo
            float progress = (GetTime() - (dungeon_->explosionEndTime - 0.5)) / 0.5f;
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
            int glyphPixelW = (int)MeasureTextEx(font, "X", (float)fontSize, 0).x;
            int glyphPixelH = drawCellH;
            int glyphX = pixelX + (drawCellW - glyphPixelW) / 2;
            int glyphY = pixelY + (drawCellH - glyphPixelH) / 2;
            DrawTextEx(font, std::string(1, glyphs[frame]).c_str(),
                      {(float)glyphX, (float)glyphY}, (float)fontSize, 0, BLACK);
        } else if (dungeon_) {
            dungeon_->explosionActive = false;
        }

        // 4) Dibujar mensaje con Raylib directo (encima de todo, sin shader)
        if (dungeon_ && !dungeon_->message.empty() && GetTime() < dungeon_->messageEndTime) {
            std::string msgText = " " + dungeon_->message + " ";
            int msgPixelW = (int)MeasureTextEx(font, msgText.c_str(), (float)fontSize, 0).x;
            int msgPixelH = cellH;

            // Calcular área del mapa según layout
            int mapPixelW, mapPixelH;
            if (hudLayout_ == HudLayout::Sidebar) {
                mapPixelW = (cols - kSidebarW - 1) * cellW;
                mapPixelH = rows * cellH;
            } else {
                mapPixelW = cols * cellW;
                mapPixelH = (rows - kHudBarH) * cellH;
            }

            // Centrado horizontalmente respecto al área del mapa
            int pixelX = offX + (mapPixelW - msgPixelW) / 2;
            // 5 filas arriba del borde inferior del mapa
            int pixelY = offY + mapPixelH - (5 * cellH) - msgPixelH;

            Color yellowBg = {255, 230, 60, 255};
            DrawRectangle(pixelX, pixelY, msgPixelW, msgPixelH, yellowBg);

            Color blackFg = {0, 0, 0, 255};
            DrawTextEx(font, msgText.c_str(), {(float)pixelX, (float)pixelY}, (float)fontSize, 0, blackFg);
        }

        // Mensaje de feedback: captura de pantalla
        if (GetTime() < screenshotMsgEndTime_)
        {
            std::string msg = " Captura guardada ";
            int msgPixelW = (int)MeasureTextEx(font, msg.c_str(), (float)fontSize, 0).x;
            int msgPixelH = cellH;
            int pixelX = (screenW - msgPixelW) / 2;
            int pixelY = screenH - cellH * 2 - msgPixelH;
            Color yellowBg = {255, 230, 60, 255};
            DrawRectangle(pixelX, pixelY, msgPixelW, msgPixelH, yellowBg);
            Color blackFg = {0, 0, 0, 255};
            DrawTextEx(font, msg.c_str(), {(float)pixelX, (float)pixelY}, (float)fontSize, 0, blackFg);
        }

        // Efecto visual de la pala: overlay marrón encima de todo
        if (shovelDigging_ && GetTime() < shovelDigEndTime_) {
            float progress = (shovelDigEndTime_ - GetTime()) / 0.6f;
            uint8_t alpha = (uint8_t)(80 * progress);
            DrawRectangle(0, 0, screenW, screenH, {139, 90, 43, alpha});
        }

        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    UnloadShader(crtShader);
    for (auto& [size, f] : fontCache)
        UnloadFont(f);
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
        {257, '\n'},           // KEY_ENTER
        {335, '\n'},           // KEY_KP_ENTER
        {256, 27},             // KEY_ESCAPE
        {258, '\t'},           // KEY_TAB
        {259, GKEY_BACKSPACE}, // KEY_BACKSPACE
        {32, ' '},             // KEY_SPACE
        // Numeros 1-6 (fila superior)
        {49, '1'},             // KEY_ONE
        {50, '2'},             // KEY_TWO
        {51, '3'},             // KEY_THREE
        {52, '4'},             // KEY_FOUR
        {53, '5'},             // KEY_FIVE
        {54, '6'},             // KEY_SIX
        // Numpad 1-6
        {320, '1'},            // KEY_KP_1
        {321, '2'},            // KEY_KP_2
        {322, '3'},            // KEY_KP_3
        {323, '4'},            // KEY_KP_4
        {324, '5'},            // KEY_KP_5
        {325, '6'},            // KEY_KP_6
    };
    // Zoom del mapa: numpad primero (antes del loop para capturar 333/334)
    if (IsKeyPressed(334)) { mapZoom_ = std::min(3, mapZoom_ + 1); saveSettings(); return; } // KP_ADD
    if (IsKeyPressed(333)) { mapZoom_ = std::max(1, mapZoom_ - 1); saveSettings(); return; } // KP_SUBTRACT

    // Detectar F12 por el archivo screenshot que crea Raylib automaticamente
    {
        char path[64];
        snprintf(path, sizeof(path), "screenshot%03i.png", screenshotCounter_);
        if (std::filesystem::exists(path))
        {
            screenshotMsgEndTime_ = GetTime() + 5.0;
            screenshotCounter_++;
        }
    }

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
        case MenuPhase::Settings:
            inputSettings(key);
            break;
        }
        break;
    case GameState::Exploration:
    {
        // No limpiar mensaje aquí, se oculta solo tras 2 segundos
        if (key == 27)
        {
            menuSelection_ = 0;
            setState(GameState::QuitDialog);
            break;
        }
        if (!dungeon_)
            break;
        Position pos = dungeon_->lock().playerPos();

        // Use a potion
        if (key == 'p' || key == 'P')
        {
            int healed = player_->useConsumable();
            if (healed > 0)
                dungeon_->message = "Usas una poción: +" + std::to_string(healed) + " HP!";
            else if (player_->countConsumables() > 0)
                dungeon_->message = "Ya tienes la vida al máximo!";
            else
                dungeon_->message = "No tienes pociones.";
            dungeon_->messageEndTime = GetTime() + 2.0;
            break;
        }

        // Use beer (Warrior) or mana potion (Mage/Ranger)
        if (key == 'r' || key == 'R')
        {
            bool used = false;
            for (const auto& item : player_->getInventory().items())
            {
                if (item.type == ItemType::Consumable && item.statBonus == 0)
                {
                    int restored = player_->getMaxMana() / 2;
                    player_->restoreMana(restored);
                    player_->getInventory().removeItem(item.name);
                    std::string label = player_->getClass() == PlayerClass::Warrior ? "Aguante" : "MP";
                    dungeon_->message = "Bebes " + item.name + ": +" + std::to_string(restored) + " " + label + "!";
                    dungeon_->messageEndTime = GetTime() + 2.0;
                    used = true;
                    break;
                }
            }
            if (!used)
                dungeon_->message = player_->getClass() == PlayerClass::Warrior
                    ? "No tienes cerveza." : "No tienes poción de mana.";
            dungeon_->messageEndTime = GetTime() + 2.0;
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

        // Open bestiary
        if (key == 'b' || key == 'B')
        {
            bestiarySelection_ = 0;
            setState(GameState::Bestiary);
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
                auto acc = dungeon_->lock();
                if (acc.isSecretWall(ax, ay))
                {
                    // Try to use bomb first, if none available reveal normally
                    int bombCount = 0;
                    for (const auto& item : player_->getInventory().items())
                        if (item.type == ItemType::Bomb) bombCount += item.quantity;
                    if (bombCount > 0) {
                        acc.map().destroyTile(ax, ay);
                        player_->getInventory().removeItem("Bomba");
                        // Activar efecto visual de explosión
                        dungeon_->explosionActive = true;
                        dungeon_->explosionEndTime = GetTime() + 0.5;
                        dungeon_->explosionX = ax;
                        dungeon_->explosionY = ay;
                        acc.map().updateFov();
                        dungeon_->message = "BOOM! La pared se hace polvo!";
                    } else {
                        dungeon_->message = "No tienes bombas.";
                        dungeon_->messageEndTime = GetTime() + 2.0;
                        return;
                    }
                    dungeon_->messageEndTime = GetTime() + 2.0;
                    found = true;
                    break;
                }
            }
            if (!found)
                dungeon_->message = "No hay nada aquí...";
            dungeon_->messageEndTime = GetTime() + 2.0;
            break;
        }

        // Movement
        int nx = pos.x, ny = pos.y;
        if (key == 'w')
            ny--;
        else if (key == 's')
            ny++;
        else if (key == 'a')
            nx--;
        else if (key == 'd')
            nx++;
        if (nx == pos.x && ny == pos.y)
            break;

        // Merchant tile → open shop
        {
            auto acc = dungeon_->lock();
            if (!acc.isWalkable(nx, ny))
                break;
            if (acc.shopExists() && nx == acc.shopMerchantPos().x && ny == acc.shopMerchantPos().y)
            {
                generateShopStock();
                shopSelection_ = 0;
                dungeon_->message.clear();
                setState(GameState::Shop);
                break;
            }
        }

        // Locked door blocks movement
        {
            auto acc = dungeon_->lock();
            if (acc.lockedDoorExists() && !acc.lockedDoorOpen() &&
                acc.lockedDoorPos().x == nx && acc.lockedDoorPos().y == ny)
            {
                dungeon_->message = "La puerta permanece sellada. Algo en las sombras aún respira.";
                dungeon_->messageEndTime = GetTime() + 2.0;
                break;
            }
        }

        // Enemy collision → combat
        bool triggered = false;
        {
            auto acc = dungeon_->lock();
            int idx = 0;
            for (auto &we : acc.enemies())
            {
                if (we.alive && we.pos.x == nx && we.pos.y == ny)
                {
                    combatWorldEnemyIdx_ = idx;
                    triggered = true;
                    break;
                }
                idx++;
            }
            if (!triggered)
            {
                acc.setPlayerPos(nx, ny);
                acc.map().updateFov();
            }
        }
        if (triggered)
        {
            setState(GameState::Combat);
            break;
        }

        // Chest on new tile
        {
            auto acc = dungeon_->lock();
            for (auto &ch : acc.chests())
            {
                if (!ch.opened && ch.pos.x == nx && ch.pos.y == ny)
                {
                    if (ch.isMimic) {
                        ch.opened = true;
                        mimicCombat_ = true;
                        setState(GameState::Combat);
                    } else {
                        openChest(ch);
                    }
                    break;
                }
            }
        }

        // Stairs on new tile
        {
            auto acc = dungeon_->lock();
            if ((!acc.lockedDoorExists() || acc.lockedDoorOpen()) &&
                acc.stairsPos().x == nx && acc.stairsPos().y == ny)
            {
                if (player_->getDungeonFloor() >= 20) {
                    victory_ = true;
                    setState(GameState::GameOver);
                } else {
                    player_->descendFloor();
                    setState(GameState::Exploration);
                    dungeon_->message = "Desciendes al piso " +
                                       std::to_string(player_->getDungeonFloor()) + "...";
                    dungeon_->messageEndTime = GetTime() + 2.0;
                    saveGame();
                }
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
    case GameState::Bestiary:
        inputBestiary(key);
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
    const int n = hs ? 5 : 4;
    switch (key)
    {
    case 'w':
    case 'a':
        menuSelection_ = (menuSelection_ - 1 + n) % n;
        break;
    case 's':
    case 'd':
        menuSelection_ = (menuSelection_ + 1) % n;
        break;
    case '\n':
        if (hs)
        {
            if (menuSelection_ == 0)
            {
                loadGame();
                loadSettings();
            }
            else if (menuSelection_ == 1)
            {
                playerName_.clear();
                menuPhase_ = MenuPhase::NameInput;
            }
            else if (menuSelection_ == 2)
            {
                menuPhase_ = MenuPhase::Settings;
            }
            else if (menuSelection_ == 3)
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
                menuPhase_ = MenuPhase::Settings;
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

void Game::inputSettings(int key)
{
    static constexpr int n = 4;
    switch (key)
    {
    case 'w':
    case 'a':
        settingsSelection_ = (settingsSelection_ - 1 + n) % n;
        break;
    case 's':
    case 'd':
        settingsSelection_ = (settingsSelection_ + 1) % n;
        break;
    case '\n':
    case ' ':
        if (settingsSelection_ == 0) {
            hudLayout_ = (hudLayout_ == HudLayout::Sidebar) ? HudLayout::Bottom : HudLayout::Sidebar;
            saveSettings();
        } else if (settingsSelection_ == 1) {
            mapZoom_ = (mapZoom_ % 3) + 1;
            saveSettings();
        } else if (settingsSelection_ == 2) {
            shaderEnabled_ = !shaderEnabled_;
            saveSettings();
        } else {
            menuSelection_ = 0;
            settingsSelection_ = 0;
            menuPhase_ = MenuPhase::Title;
        }
        break;
    case 27:
        menuSelection_ = 0;
        settingsSelection_ = 0;
        menuPhase_ = MenuPhase::Title;
        break;
    }
}

void Game::inputQuitDialog(int key)
{
    switch (key)
    {
    case 'w':
    case 'a':
        menuSelection_ = (menuSelection_ - 1 + 2) % 2;
        break;
    case 's':
    case 'd':
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
            secretEasterEgg_ = (playerName_ == "halley" || playerName_ == "nato"
                                || playerName_ == "halley&nato");
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
    if (secretEasterEgg_) {
        switch (key) {
        case 'a': classSelection_ = 0; break;
        case 'd': classSelection_ = 1; break;
        case 27:
            secretEasterEgg_ = false;
            menuPhase_ = MenuPhase::NameInput;
            return;
        case '\n':
            hudSelection_ = 0;
            menuPhase_ = MenuPhase::HudSelect;
            return;
        }
    } else {
        switch (key) {
        case 'w': case 's':
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
}

void Game::inputHudSelect(int key)
{
    switch (key)
    {
    case 'a':
    case 'd':
        hudSelection_ = (hudSelection_ + 1) % 2;
        break;
    case 27:
        menuPhase_ = MenuPhase::ClassSelect;
        break;
    case '\n':
    {
        hudLayout_ = (hudSelection_ == 0) ? HudLayout::Sidebar : HudLayout::Bottom;
        PlayerClass cls;
        if (secretEasterEgg_) {
            cls = (classSelection_ == 0) ? PlayerClass::Halley : PlayerClass::Nato;
        } else {
            switch (classSelection_)
            {
            case 0:  cls = PlayerClass::Warrior; break;
            case 1:  cls = PlayerClass::Mage;    break;
            default: cls = PlayerClass::Ranger;  break;
            }
        }
        player_ = std::make_unique<Player>(playerName_, cls);
        // Dar 2 bombas iniciales (como 1 item con quantity=2)
        {
            Item bomba = DungeonPopulator::pickBomb(1);
            bomba.quantity = 2;
            player_->getInventory().addItem(bomba);
        }
        initBestiary();
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
    if (dungeon_ && state_.load() == GameState::Exploration && dungeon_->allEnemiesDead())
    {
        dungeon_->openLockedDoor();
        dungeon_->message = "El silencio se apodera del Tenebrarium... algo cede en la oscuridad.";
        dungeon_->messageEndTime = GetTime() + 2.0;
    }
}

void Game::initBestiary()
{
    for (int i = 0; i < kBestiaryEntryCount; i++) {
        bestiary_[i].type = static_cast<EnemyType>(i);
        bestiary_[i].name = kBestiaryData[i].name;
        bestiary_[i].kills = 0;
        bestiary_[i].encountered = 0;
        bestiary_[i].firstSeenFloor = 999;
        bestiary_[i].discovered = false;
    }
}

void Game::initQuests()
{
    quests_.clear();
    if (dungeon_) {
        dungeon_->enemiesKilled = 0;
        dungeon_->chestsOpened = 0;
    }

    quests_.push_back({"primer_sangre", "Primer Sangre", "Derrota a tu primer enemigo en el Tenebrarium.", QuestStatus::InProgress, {{"Mata 1 enemigo", false}}, 50, 0});
    quests_.push_back({"cazador", "Cazador", "Los monstruos del Tenebrarium son peligrosos. Demuestra que no te amedrentan.", QuestStatus::InProgress, {{"Mata 5 enemigos", false}}, 150, 20});
    quests_.push_back({"buscador", "Buscador de Tesoros", "Las criptas están llenas de riquezas olvidadas. Encuéntralas.", QuestStatus::InProgress, {{"Abre 3 cofres", false}}, 100, 30});
    quests_.push_back({"descenso", "Descenso a las Profundidades", "Las criaturas más peligrosas habitan los pisos inferiores.", QuestStatus::InProgress, {{"Llega al piso 2", false}, {"Llega al piso 3", false}}, 200, 50});
}

void Game::checkQuestProgress()
{
    if (!player_ || !dungeon_)
        return;
    int floor = player_->getDungeonFloor();

    for (auto &q : quests_)
    {
        if (q.status == QuestStatus::Completed || q.status == QuestStatus::Failed)
            continue;
        if (q.id == "primer_sangre")
        {
            q.objectives[0].completed = (dungeon_->enemiesKilled >= 1);
        }
        else if (q.id == "cazador")
        {
            q.objectives[0].completed = (dungeon_->enemiesKilled >= 5);
        }
        else if (q.id == "buscador")
        {
            q.objectives[0].completed = (dungeon_->chestsOpened >= 3);
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
            dungeon_->message = "Misión completada: " + q.title + "!";
            dungeon_->messageEndTime = GetTime() + 2.0;
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
            Renderer::drawClassSelect(scr, classSelection_, secretEasterEgg_);
            break;
        case MenuPhase::HudSelect:
            Renderer::drawHudSelect(scr, hudSelection_);
            break;
        case MenuPhase::Settings:
            Renderer::drawSettings(scr, settingsSelection_, hudLayout_, mapZoom_, shaderEnabled_);
            break;
        }
        break;
    case GameState::Exploration:
        if (dungeon_ && player_)
        {
            std::vector<MapEntity> entities;
            {
                auto acc = dungeon_->lock();
                for (const auto &we : acc.enemies())
                    if (we.alive)
                        entities.push_back({we.pos, glyphForEnemy(we.type), colorPairForEnemy(we.type), true});
                for (const auto &ch : acc.chests())
                    if (!ch.opened)
                        entities.push_back({ch.pos, '$', 2, true});
                if (acc.lockedDoorExists() && !acc.lockedDoorOpen())
                    entities.push_back({acc.lockedDoorPos(), '+', 1, false});
                if (!acc.lockedDoorExists() || acc.lockedDoorOpen())
                    entities.push_back({acc.stairsPos(), '>', 3, true, true});
                if (acc.shopExists())
                    entities.push_back({acc.shopMerchantPos(), '$', 4, true});
                Renderer::drawExploration(scr, acc.map(), *player_, hudLayout_,
                                           entities, "", mapZoom_, scrollTick_);
                scrollTick_++;
             }
         }
        break;
    case GameState::Combat:
        if (combat_ && player_ && dungeon_)
        {
            bool boss = false;
            if (combatWorldEnemyIdx_ >= 0) {
                auto acc = dungeon_->lock();
                if (combatWorldEnemyIdx_ < static_cast<int>(acc.enemies().size()))
                    boss = acc.enemies()[combatWorldEnemyIdx_].isBoss;
            }
            int flashIdx = (GetTime() < combatFlashEndTime_) ? combatFlashIdx_ : -1;
            Renderer::drawCombat(scr, *combat_, *player_,
                                 combatShowingArts_, combatArtSelection_, boss, flashIdx);
        }
        break;
    case GameState::Shop:
        if (player_)
            Renderer::drawShop(scr, shopStock_, shopSelection_,
                               *player_, dungeon_ ? dungeon_->message : "",
                               shopSellMode_, shopSellSelection_);
        break;
    case GameState::Inventory:
        if (player_)
            Renderer::drawInventory(scr, *player_, inventorySelection_);
        break;
    case GameState::QuestLog:
        Renderer::drawQuestLog(scr, quests_, questLogSelection_);
        break;
    case GameState::Bestiary:
        Renderer::drawBestiary(scr, bestiary_, bestiarySelection_);
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

static int glyphForEnemy(EnemyType t)
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
    case EnemyType::Mimic:    return 'M';
    case EnemyType::Slime:    return 0x25CF; // ●
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
    case EnemyType::Mimic:    return 9; // COL_ORANGE
    case EnemyType::Slime:    return 10; // COL_DK_GREEN
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
        // No guardar si venimos de GameOver (muerte/victoria) — ya se borró el save
        if (state_.load() != GameState::GameOver && player_ && dungeon_)
            saveGame();
        pendingCombatEnemy_.store(-1);
        victory_ = false;
        menuPhase_ = MenuPhase::Title;
        menuSelection_ = 0;
        secretEasterEgg_ = false;
        classSelection_ = 0;
        hudSelection_ = 0;
        playerName_.clear();
        player_.reset();
        dungeon_.reset();
        combat_.reset();
    }

    if (newState == GameState::Exploration)
    {
        pendingCombatEnemy_.store(-1);
        mimicCombat_ = false;
        combat_.reset();
        shopStock_.clear();  // el stock se regenera la primera vez que se entra a la tienda del nuevo piso


        int floor = player_ ? player_->getDungeonFloor() : 1;
        PlayerClass cls = player_ ? player_->getClass() : PlayerClass::Warrior;

        dungeon_ = std::make_unique<Dungeon>();
        dungeon_->generate(floor, cls);
    }

    if (newState == GameState::Combat && player_ && dungeon_)
    {
        combatShowingArts_ = false;
        combatArtSelection_ = 0;

        // Instanciar el Enemy de combate a partir del WorldEnemy del mundo.
        // Si combatWorldEnemyIdx_ == -1 (caso raro/debug) se usan dos enemigos por defecto.
        std::vector<std::unique_ptr<Enemy>> enemies;
        int fl = player_ ? player_->getDungeonFloor() : 1;
        if (mimicCombat_) {
            enemies.push_back(DungeonPopulator::makeEnemy(EnemyType::Mimic, fl));
            mimicCombat_ = false;
        } else if (combatWorldEnemyIdx_ >= 0)
        {
            auto acc = dungeon_->lock();
            if (combatWorldEnemyIdx_ < static_cast<int>(acc.enemies().size())) {
                bool boss = acc.enemies()[combatWorldEnemyIdx_].isBoss;
                enemies.push_back(DungeonPopulator::makeEnemy(
                    acc.enemies()[combatWorldEnemyIdx_].type, fl, boss));
                if (!boss && enemies.back()->getType() == EnemyType::Spider && (std::rand() % 100) < 1)
                    enemies.back()->setName("Ariatña");
            }
        }
        else
        {
            enemies.push_back(DungeonPopulator::makeEnemy(EnemyType::Goblin, fl));
            enemies.push_back(DungeonPopulator::makeEnemy(EnemyType::Skeleton, fl));
        }
        combat_ = std::make_unique<CombatSystem>(*player_, std::move(enemies));

        // Bestiary: marcar enemigos como encontrados
        for (const auto& e : combat_->getEnemies()) {
            int idx = static_cast<int>(e->getType());
            if (idx >= 0 && idx < kBestiaryEntryCount) {
                auto& be = bestiary_[idx];
                if (!be.discovered) {
                    be.discovered = true;
                    be.firstSeenFloor = player_->getDungeonFloor();
                }
                be.encountered++;
            }
        }
    }

    if (newState == GameState::GameOver)
        GameSerializer::deleteSave();

    state_.store(newState);
}

void Game::openChest(WorldChest& chest)
{
    chest.opened = true;
    if (!dungeon_) return;
    dungeon_->chestsOpened++;

    switch (chest.loot)
    {
    case ChestLoot::Coins:
        player_->addCoins(chest.coins);
        dungeon_->message = "Cofre: +" + std::to_string(chest.coins) + " monedas de oro!";
        dungeon_->messageEndTime = GetTime() + 2.0;
        break;
    case ChestLoot::Item:
        if (chest.item.type == ItemType::Consumable)
        {
            player_->pickupItem(chest.item);
            dungeon_->message = "Cofre: encontraste " + chest.item.name + "!";
        }
        else
        {
            player_->pickupItem(chest.item);
            dungeon_->message = "Cofre: encontraste " + chest.item.name + "!  (+" +
                std::to_string(chest.item.statBonus) +
                (chest.item.type == ItemType::Weapon ? " ATK" : " DEF") + ")";
        }
        dungeon_->messageEndTime = GetTime() + 2.0;
        break;
    }
}


void Game::returnToExploration()
{
    // Return to the same map without regenerating — just clear combat state
    combat_.reset();
    combatWorldEnemyIdx_ = -1;
    if (dungeon_) dungeon_->message.clear();
    state_.store(GameState::Exploration);
}

void Game::inputInventory(int key)
{
    if (!player_)
        return;
    const int bagSize = static_cast<int>(player_->getInventory().items().size());
    const int total = bagSize;
    switch (key)
    {
    case 'w':
    case 's':
        navV(key, inventorySelection_, total);
        break;
    case 'e':
    case 'E':
    case '\n':
    {
        int bagIdx = inventorySelection_;
        if (bagIdx >= 0 && bagIdx < bagSize)
        {
            player_->equipItem(bagIdx);
            int newSize = static_cast<int>(player_->getInventory().items().size());
            if (inventorySelection_ >= newSize)
                inventorySelection_ = std::max(0, newSize - 1);
        }
        break;
    }
    case 'u':
    case 'U':
    {
        int bagIdx = inventorySelection_;
        if (bagIdx >= 0 && bagIdx < bagSize)
        {
            auto& items = player_->getInventory().items();
            const auto& item = items[bagIdx];
            if (item.type == ItemType::Consumable && item.statBonus > 0)
            {
                int healed = std::min(item.statBonus, player_->getMaxHp() - player_->getHp());
                if (healed > 0) {
                    player_->heal(healed);
                    player_->getInventory().removeItem(item.name);
                    if (dungeon_) {
                        dungeon_->message = "Usas " + item.name + ": +" + std::to_string(healed) + " HP!";
                        dungeon_->messageEndTime = GetTime() + 2.0;
                    }
                }
            }
            if (item.name == "Pala")
            {
                useShovel();
                return;
            }
        }
        break;
    }
    case 27:
    case 'q':
    case 'Q':
        state_.store(GameState::Exploration, std::memory_order_release);
        break;
    }
}

void Game::inputQuestLog(int key)
{
    int n = static_cast<int>(quests_.size());
    switch (key)
    {
    case 'w':
    case 's':
        if (n > 0) navV(key, questLogSelection_, n);
        break;
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
        break;
    }
}

void Game::inputBestiary(int key)
{
    switch (key)
    {
    case 'w':
    case 's':
        navV(key, bestiarySelection_, kBestiaryEntryCount);
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

    if (shopSellMode_)
    {
        switch (key)
        {
        case 'w':
        case 's':
        {
            auto& inv = player_->getInventory().items();
            int dir = (key == 'w') ? -1 : 1;
            int cur = shopSellSelection_;
            int steps = 0;
            while (steps < static_cast<int>(inv.size()))
            {
                cur += dir;
                if (cur < 0) cur = static_cast<int>(inv.size()) - 1;
                if (cur >= static_cast<int>(inv.size())) cur = 0;
                if (inv[cur].type == ItemType::Weapon || inv[cur].type == ItemType::Armor)
                {
                    shopSellSelection_ = cur;
                    break;
                }
                steps++;
            }
            break;
        }
        case '\n':
        {
            auto& inv = player_->getInventory().items();
            int idx = shopSellSelection_;
            if (idx >= 0 && idx < static_cast<int>(inv.size()))
            {
                auto& item = inv[idx];
                if (item.type == ItemType::Weapon || item.type == ItemType::Armor)
                {
                    int sellPrice = item.value / 2;
                    player_->addCoins(sellPrice);
                    player_->getInventory().removeItem(item.name);
                    if (dungeon_) {
                        dungeon_->message = "Vendiste " + item.name + " por " +
                                            std::to_string(sellPrice) + " monedas!";
                        dungeon_->messageEndTime = GetTime() + 2.0;
                    }
                    if (shopSellSelection_ >= static_cast<int>(inv.size()))
                        shopSellSelection_ = std::max(0, static_cast<int>(inv.size()) - 1);
                }
            }
            break;
        }
        case 'v':
        case 'V':
            shopSellMode_ = false;
            break;
        case 27:
        case 'q':
        case 'Q':
            shopSellMode_ = false;
            state_ = GameState::Exploration;
            break;
        }
        return;
    }

    switch (key)
    {
    case 'w':
    case 's':
        navV(key, shopSelection_, n);
        break;
    case '\n':
    {
        auto &s = shopStock_[shopSelection_];
        if (s.sold)
        {
            if (dungeon_) dungeon_->message = "Ya vendido.";
            break;
        }
        if (player_->getCoins() < s.price)
        {
            if (dungeon_) {
                dungeon_->message = "No tienes suficiente oro.";
                dungeon_->messageEndTime = GetTime() + 2.0;
            }
            break;
        }
        player_->addCoins(-s.price);
        if (s.item.type == ItemType::Consumable || s.item.type == ItemType::Bomb)
            player_->getInventory().addItem(s.item);
        else
            player_->pickupItem(s.item);
        s.sold = true;
        if (dungeon_) {
            dungeon_->message = "Compraste: " + s.item.name + "!";
            dungeon_->messageEndTime = GetTime() + 2.0;
        }
        break;
    }
    case 'v':
    case 'V':
        shopSellMode_ = true;
        shopSellSelection_ = 0;
        // Ajustar al primer item vendible
        {
            auto& inv = player_->getInventory().items();
            for (int i = 0; i < static_cast<int>(inv.size()); i++)
            {
                if (inv[i].type == ItemType::Weapon || inv[i].type == ItemType::Armor)
                {
                    shopSellSelection_ = i;
                    break;
                }
            }
        }
        break;
    case 27:
    case 'q':
    case 'Q':
        state_ = GameState::Exploration;
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
    if (cls == PlayerClass::Warrior) {
        Item beer = DungeonPopulator::pickBeer();
        beer.quantity = 3;
        shopStock_.push_back({beer, 5, false});
    } else {
        Item manaPot = DungeonPopulator::pickManaPotion();
        manaPot.quantity = 3;
        shopStock_.push_back({manaPot, 8, false});
    }
    Item bomb = DungeonPopulator::pickBomb(shopFloor);
    bomb.quantity = 3; // Vender en grupos de 3
    shopStock_.push_back({bomb, bomb.value, false});
    Item shovel = DungeonPopulator::pickShovel(shopFloor);
    shopStock_.push_back({shovel, shovel.value, false});
    Item w = DungeonPopulator::pickWeapon(cls, shopFloor);
    shopStock_.push_back({w, w.value + shopFloor * 5, false});
    Item a = DungeonPopulator::pickArmor(cls, shopFloor);
    shopStock_.push_back({a, a.value + shopFloor * 5, false});
}

bool Game::isInShopRoom(Dungeon::Lock& acc, Position p) const
{
    return acc.isInShopRoom(p);
}

void Game::useBomb(Dungeon::Lock& acc)
{
    for (const auto &item : player_->getInventory().items())
    {
        if (item.type == ItemType::Bomb)
        {
            player_->getInventory().removeItem(item.name);
            const int sdx[] = {0, 0, -1, 1};
            const int sdy[] = {-1, 1, 0, 0};
            Position pos = acc.playerPos();
            for (int i = 0; i < 4; i++)
            {
                int ax = pos.x + sdx[i], ay = pos.y + sdy[i];
                if (acc.isSecretWall(ax, ay) && acc.map().destroyTile(ax, ay))
                {
                    dungeon_->explosionActive = true;
                    dungeon_->explosionEndTime = GetTime() + 0.5;
                    dungeon_->explosionX = ax;
                    dungeon_->explosionY = ay;
                    acc.map().updateFov();
                    dungeon_->message = "La bomba destruye la pared!";
                    dungeon_->messageEndTime = GetTime() + 2.0;
                    return;
                }
            }
            dungeon_->message = "No hay paredes secretas adyacentes.";
            dungeon_->messageEndTime = GetTime() + 2.0;
            return;
        }
    }
    dungeon_->message = "No tienes bombas.";
    dungeon_->messageEndTime = GetTime() + 2.0;
}

void Game::useShovel()
{
    player_->getInventory().removeItem("Pala");
    player_->descendFloor();
    if (player_->getDungeonFloor() > 20) {
        victory_ = true;
        setState(GameState::GameOver);
        return;
    }
    setState(GameState::Exploration);
    shovelDigging_ = true;
    shovelDigEndTime_ = GetTime() + 0.6;
    if (dungeon_) {
        dungeon_->message = "Excavas hacia el piso " + std::to_string(player_->getDungeonFloor()) + "...";
        dungeon_->messageEndTime = GetTime() + 2.0;
    }
    saveGame();
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
                auto acc = dungeon_->lock();
                auto &we = acc.enemies()[combatWorldEnemyIdx_];
                int ei = static_cast<int>(we.type);
                if (ei >= 0 && ei < kBestiaryEntryCount) {
                    bestiary_[ei].kills++;
                    bestiary_[ei].discovered = true;
                }
                int baseXp = DungeonPopulator::xpForEnemy(we.type, player_->getDungeonFloor());
                int xpGained = we.isBoss ? baseXp * 5 : baseXp;
                player_->gainXp(xpGained);
                we.alive = false;
                dungeon_->enemiesKilled++;
                int lootChance = we.isBoss ? 45 : 15;
                // chance the enemy drops a potion
                if (std::rand() % 100 < lootChance)
                {
                    Item pot = DungeonPopulator::pickPotion(player_->getDungeonFloor());
                    player_->getInventory().addItem(pot);
                    dungeon_->message = "El enemigo soltó una " + pot.name + "!";
                    dungeon_->messageEndTime = GetTime() + 2.0;
                }
            }
            // Regeneración post-combate
            int hpRegen = std::max(1, player_->getMaxHp() * 15 / 100);
            player_->heal(hpRegen);
            int mpRegen = std::max(1, player_->getMaxMana() * (player_->getClass() == PlayerClass::Warrior ? 5 : 10) / 100);
            player_->restoreMana(mpRegen);
            if (dungeon_->message.empty())
                dungeon_->message = "Recuperas " + std::to_string(hpRegen) + " HP y " + std::to_string(mpRegen) + " MP.";
        dungeon_->messageEndTime = GetTime() + 2.0;
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
        case 'w':
        case 's':
            navV(key, combatArtSelection_, n);
            break;
        case '1': case '2': case '3': {
            int idx = key - '1';
            if (idx < n) {
                combat_->doArt(idx);
                combatFlashIdx_ = combat_->getCurrentTarget();
                combatFlashEndTime_ = GetTime() + 0.25;
                combatShowingArts_ = false;
            }
            break;
        }
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
    case '1':
        combat_->doAttack();
        combatFlashIdx_ = combat_->getCurrentTarget();
        combatFlashEndTime_ = GetTime() + 0.25;
        break;
    case '2':
        combat_->doHeavyAttack();
        combatFlashIdx_ = combat_->getCurrentTarget();
        combatFlashEndTime_ = GetTime() + 0.25;
        break;
    case '3':
        combatShowingArts_ = true;
        combatArtSelection_ = 0;
        break;
    case '4':
        combat_->doDefend();
        break;
    case '5':
        combat_->doUseItem();
        break;
    case '6':
        combat_->doFlee();
        break;
    case '7':
        combat_->doLoot();
        combatFlashIdx_ = combat_->getCurrentTarget();
        combatFlashEndTime_ = GetTime() + 0.25;
        break;
    case ' ':
        combat_->doEndTurn();
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
    if (f) {
        f << "mapZoom=" << mapZoom_ << "\n";
        f << "hudLayout=" << static_cast<int>(hudLayout_) << "\n";
        f << "shaderEnabled=" << static_cast<int>(shaderEnabled_) << "\n";
    }
}

void Game::loadSettings() {
    std::ifstream f(std::string(GetApplicationDirectory()) + "saves/settings.dat");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("mapZoom=", 0) == 0) {
            try { mapZoom_ = std::clamp(std::stoi(line.substr(8)), 1, 3); }
            catch (...) {}
        } else if (line.rfind("hudLayout=", 0) == 0) {
            try { hudLayout_ = static_cast<HudLayout>(std::stoi(line.substr(10))); }
            catch (...) {}
        } else if (line.rfind("shaderEnabled=", 0) == 0) {
            try { shaderEnabled_ = std::stoi(line.substr(14)) != 0; }
            catch (...) {}
        }
    }
}
