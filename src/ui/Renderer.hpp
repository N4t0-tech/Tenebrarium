#pragma once

#include "world/Map.hpp"
#include "entities/Player.hpp"
#include "combat/CombatSystem.hpp"
#include "ui/HudLayout.hpp"
#include "core/ShopItem.hpp"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

class Renderer {
public:
    // Menu screens
    static ftxui::Element drawTitle(int selection);
    static ftxui::Element drawNameInput(const std::string& name);
    static ftxui::Element drawClassSelect(int selection);
    static ftxui::Element drawHudSelect(int selection);   // 0=Sidebar, 1=Bottom

    // Game screens
    static ftxui::Element drawExploration(const Map& map, const Player& player, HudLayout layout,
                                          const std::vector<MapEntity>& entities,
                                          const std::string& message = "");
    static ftxui::Element drawCombat(const CombatSystem& combat, const Player& player,
                                     bool showingArts, int artSelection);
    static ftxui::Element drawInventory(const Player& player, int selection);
    static ftxui::Element drawQuestLog();
    static ftxui::Element drawGameOver();
    static ftxui::Element drawShop(const std::vector<ShopItem>& stock,
                                   int selection, const Player& player,
                                   const std::string& message);

private:
    static ftxui::Element drawExplorationSidebar(const Map& map, const Player& player,
                                                  const std::vector<MapEntity>& entities,
                                                  const std::string& message);
    static ftxui::Element drawExplorationBottom(const Map& map, const Player& player,
                                                 const std::vector<MapEntity>& entities,
                                                 const std::string& message);
    static ftxui::Element makeMap(const Map& map,
                                   const std::vector<MapEntity>& entities);
    static ftxui::Element makeStatBar(int value, int max, ftxui::Color col);
    static ftxui::Element makeHudPanel(const Player& player);
    static ftxui::Element makeHudBar(const Player& player);
};
