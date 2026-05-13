#pragma once
#include <string>
#include "raylib.h"
#include "HudLayout.hpp"

class RayguiGUI {
public:
    static void init(Font font, int fontSize);
    static void begin();
    static void end();

    static void drawSettings(int selection, HudLayout hud, int mapZoom, bool shaderOn);
    static void drawGameOver(bool victory);
    static void drawQuitDialog(int selection);

    // Fase 2 — Menús principales
    static void drawTitle(int selection, bool hasSave, bool blink);
    static void drawCredits();
    static void drawNameInput(const std::string& name, bool blink);
    static void drawClassSelect(int selection);
    static void drawHudSelect(int selection);
};
