#pragma once

#include "world/Map.hpp"
#include "entities/Player.hpp"
#include "ui/HudLayout.hpp"
#include <string>

class Renderer {
public:
    // Menu screens
    static void drawTitle(int selection);
    static void drawNameInput(const std::string& name);
    static void drawClassSelect(int selection);
    static void drawHudSelect(int selection);   // 0=Sidebar, 1=Bottom

    // Game screens
    static void drawExploration(const Map& map, const Player& player, HudLayout layout);
    static void drawCombat();
    static void drawInventory();
    static void drawQuestLog();
    static void drawGameOver();

    // Shared helpers
    static void drawBox(int y, int x, int h, int w);
    static void drawCentered(int row, const std::string& text);

private:
    static void drawExplorationSidebar(const Map& map, const Player& player);
    static void drawExplorationBottom(const Map& map, const Player& player);
    static void drawMap(const Map& map, const Player& player, int viewX, int viewY, int viewW, int viewH);
    static void drawHudBar(const Player& player, int startY, int startX, int w);
    static void drawHudPanel(const Player& player, int startX, int w, int h);
    static void drawStatBar(int y, int x, int w, int value, int max, int colorPair);
};
