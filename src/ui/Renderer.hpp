#pragma once

// Renderer — todos los métodos son estáticos y reciben un TerminalScreen& donde dibujar.
// No llaman a Raylib directamente; usan scr.putStr/put/putCell para llenar el buffer.
// Esto permite testar el layout sin ventana y hace el renderer stateless (fácil de extender).
// Para agregar una pantalla nueva: agregar método drawXxx() aquí, implementarlo en Renderer.cpp
// y llamarlo desde Game::render() en el case correspondiente.

#include "ui/TerminalScreen.hpp"
#include "world/Map.hpp"
#include "entities/Player.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/HudLayout.hpp"
#include "core/ShopItem.hpp"
#include "quests/Quest.hpp"
#include <string>
#include <vector>

class Renderer {
public:
    // ── Menús ────────────────────────────────────────────────────────────────
    static void drawTitle(TerminalScreen& scr, int selection, bool hasSave, bool blink);
    static void drawCredits(TerminalScreen& scr);
    static void drawNameInput(TerminalScreen& scr, const std::string& name, bool blink);
    static void drawClassSelect(TerminalScreen& scr, int selection, bool secretUnlocked = false);
    static void drawHudSelect(TerminalScreen& scr, int selection);
    static void drawSettings(TerminalScreen& scr, int selection, HudLayout hud,
                             int mapZoom, bool shaderOn);

    // ── Pantallas de juego ───────────────────────────────────────────────────
    static void drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message = "",
                                int mapZoom = 1);
    static Color colorForPlayerClass(PlayerClass pc);
    static void drawMap(TerminalScreen& scr, int col, int row, int viewW, int viewH,
                        const Map& map, const std::vector<MapEntity>& entities,
                        Color playerColor);
    static void drawCombat(TerminalScreen& scr, const CombatSystem& combat,
                           const Player& player, bool showingArts, int artSelection,
                           bool isBoss = false, int flashIdx = -1);
    static void drawInventory(TerminalScreen& scr, const Player& player, int selection);
    static void drawQuestLog(TerminalScreen& scr, const std::vector<Quest>& quests, int selection);
    static void drawGameOver(TerminalScreen& scr, bool victory = false);
    static void drawQuitDialog(TerminalScreen& scr, int selection);
    static void drawShop(TerminalScreen& scr, const std::vector<ShopItem>& stock,
                         int selection, const Player& player,
                         const std::string& message,
                         bool sellMode = false, int sellSelection = 0);

private:
    // ── Primitivas de layout ─────────────────────────────────────────────────
    static void drawBorder(TerminalScreen& scr, int col, int row, int w, int h,
                           Color c = WHITE);
    static void drawHSep(TerminalScreen& scr, int col, int row, int w,
                         Color c = WHITE);
    static void drawVSep(TerminalScreen& scr, int col, int row, int h,
                         Color c = WHITE);
    static void drawCentered(TerminalScreen& scr, int row, int col0, int w,
                             const std::string& text, Color fg, uint8_t flags = 0);
    // drawStatBar: rellena barW celdas con █ (llenas) o ░ (vacías) según value/max
    static void drawStatBar(TerminalScreen& scr, int col, int row,
                            int value, int max, int barW, Color c);

    static Color colorFromPair(int pair);
    static void  drawHudPanel(TerminalScreen& scr, int col, int row,
                              const Player& player, int mapZoom = 1);
    static void  drawHudBar(TerminalScreen& scr, int row,
                             const Player& player, int mapZoom = 1);
    static void  drawPortrait(TerminalScreen& scr, int col, int row,
                               const char* filename, float scale = 1.0f,
                               Color tint = WHITE);
    static const char* className(const Player& p);
};
