#pragma once

#include "world/Map.hpp"
#include "entities/Player.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/HudLayout.hpp"
#include <string>
#include <vector>

class Renderer {
public:
    // Menu screens
    static void drawTitle(int selection);
    static void drawNameInput(const std::string& name);
    static void drawClassSelect(int selection);
    static void drawHudSelect(int selection);   // 0=Sidebar, 1=Bottom

    // Game screens
    static void drawExploration(const Map& map, const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message = "");
    static void drawCombat(const CombatSystem& combat, const Player& player,
                           bool showingArts, int artSelection);
    static void drawInventory();
    static void drawQuestLog();
    static void drawGameOver();

    // Shared helpers
    static void drawBox(int y, int x, int h, int w);
    static void drawCentered(int row, const std::string& text);

private:
    static void drawExplorationSidebar(const Map& map, const Player& player,
                                       const std::vector<MapEntity>& entities,
                                       const std::string& message);
    static void drawExplorationBottom(const Map& map, const Player& player,
                                      const std::vector<MapEntity>& entities,
                                      const std::string& message);
    static void drawMap(const Map& map, const Player& player, int viewX, int viewY, int viewW, int viewH,
                        const std::vector<MapEntity>& entities);
    static void drawHudBar(const Player& player, int startY, int startX, int w);
    static void drawHudPanel(const Player& player, int startX, int w, int h);
    static void drawStatBar(int y, int x, int w, int value, int max, int colorPair);
};
