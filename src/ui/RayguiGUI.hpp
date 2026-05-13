#pragma once
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
};
