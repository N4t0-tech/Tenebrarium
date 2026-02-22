#pragma once

#include <string>

class Renderer {
public:
    // Menu screens
    static void drawTitle(int selection);               // 0=Nueva Partida, 1=Salir
    static void drawNameInput(const std::string& name);
    static void drawClassSelect(int selection);         // 0=Warrior, 1=Mage, 2=Ranger

    // Game screens
    static void drawExploration();
    static void drawCombat();
    static void drawInventory();
    static void drawQuestLog();
    static void drawGameOver();

    // Shared helpers
    static void drawBox(int y, int x, int h, int w);
    static void drawCentered(int row, const std::string& text);
};
