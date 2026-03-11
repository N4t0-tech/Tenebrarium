#pragma once

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
    // Menu screens
    static void drawTitle(TerminalScreen& scr, int selection);
    static void drawNameInput(TerminalScreen& scr, const std::string& name);
    static void drawClassSelect(TerminalScreen& scr, int selection);
    static void drawHudSelect(TerminalScreen& scr, int selection);

    // Game screens
    static void drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message = "");
    static void drawCombat(TerminalScreen& scr, const CombatSystem& combat,
                           const Player& player, bool showingArts, int artSelection);
    static void drawInventory(TerminalScreen& scr, const Player& player, int selection);
    static void drawQuestLog(TerminalScreen& scr, const std::vector<Quest>& quests, int selection);
    static void drawGameOver(TerminalScreen& scr);
    static void drawShop(TerminalScreen& scr, const std::vector<ShopItem>& stock,
                         int selection, const Player& player,
                         const std::string& message);

private:
    // Layout primitives
    static void drawBorder(TerminalScreen& scr, int col, int row, int w, int h,
                           Color c = WHITE);
    static void drawHSep(TerminalScreen& scr, int col, int row, int w,
                         Color c = WHITE);
    static void drawVSep(TerminalScreen& scr, int col, int row, int h,
                         Color c = WHITE);
    static void drawCentered(TerminalScreen& scr, int row, int col0, int w,
                             const std::string& text, Color fg, uint8_t flags = 0);
    static void drawStatBar(TerminalScreen& scr, int col, int row,
                            int value, int max, int barW, Color c);

    static Color colorFromPair(int pair);
    static void  drawMap(TerminalScreen& scr, int col, int row, int viewW, int viewH,
                         const Map& map, const std::vector<MapEntity>& entities);
    static void  drawHudPanel(TerminalScreen& scr, int col, int row,
                              const Player& player);
    static void  drawHudBar(TerminalScreen& scr, int row, const Player& player);
    static void  drawPortrait(TerminalScreen& scr, int col, int row,
                              const char* filename);
    static const char* className(const Player& p);
};
