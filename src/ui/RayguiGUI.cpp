#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "RayguiGUI.hpp"
#include <string>
#include <fstream>

static constexpr Color COL_WHITE   = WHITE;
static constexpr Color COL_YELLOW  = { 255, 230,  60, 255 };
static constexpr Color COL_CYAN    = {  60, 230, 240, 255 };
static constexpr Color COL_GREEN   = {  60, 230, 110, 255 };
static constexpr Color COL_RED     = { 255,  80,  80, 255 };
static constexpr Color COL_GRAY    = { 200, 200, 200, 255 };
static constexpr Color COL_ORANGE  = { 255, 185,  40, 255 };
static constexpr Color COL_BLACK   = BLACK;

static Rectangle rec(int x, int y, int w, int h) {
    return {(float)x, (float)y, (float)w, (float)h};
}

static std::string assetsDir() {
    return std::string(GetApplicationDirectory()) + "assets/";
}

// ─── Init ────────────────────────────────────────────────────────────────────

void RayguiGUI::init(Font font, int fontSize) {
    GuiSetFont(font);

    GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize);

    // Background
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(COL_BLACK));

    // Normal state
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   ColorToInt(COL_WHITE));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(COL_GRAY));

    // Focused / selected state
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,   ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,   ColorToInt(COL_YELLOW));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(COL_YELLOW));

    // Pressed state
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,   ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,   ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(COL_GRAY));

    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
}

void RayguiGUI::begin() {
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
}

void RayguiGUI::end() {
    // no-op por ahora
}

// ─── Settings ────────────────────────────────────────────────────────────────

void RayguiGUI::drawSettings(int selection, HudLayout hud, int mapZoom, bool shaderOn) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    int cx = sw / 2;
    int cy = sh / 2;

    // Title
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    GuiLabel(rec(cx - 100, cy - 120, 200, 30), "T E N E B R A R I U M");
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    GuiLabel(rec(cx - 100, cy - 90, 200, 25), "Configuracion");

    const char* items[] = {
        "Estilo HUD",
        "Zoom Mapa",
        "Shader CRT",
        "Volver",
    };

    const char* vals[] = {
        (hud == HudLayout::Sidebar) ? "Panel lateral" : "Barra inferior",
        (mapZoom == 1) ? "1x" : (mapZoom == 2) ? "2x" : "3x",
        shaderOn ? "On" : "Off",
        "",
    };

    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    for (int i = 0; i < 4; i++) {
        int y = cy - 50 + i * 35;

        if (selection == i) {
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_YELLOW));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_BLACK));
        } else {
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
        }

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiLabel(rec(cx - 80, y, 160, 25), items[i]);

        if (vals[i][0] != '\0') {
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
            GuiLabel(rec(cx + 20, y, 60, 25), vals[i]);
        }
    }

    // Reset style
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    GuiLabel(rec(cx - 120, cy + 100, 240, 20),
             "W/S navegar  |  ENTER cambiar  |  ESC volver");
}

// ─── Game Over ───────────────────────────────────────────────────────────────

void RayguiGUI::drawGameOver(bool victory) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    std::string filename = assetsDir() + (victory ? "victoryTitle.txt" : "gameOverTitle.txt");
    std::ifstream f(filename);
    std::string line;
    Font font = GuiGetFont();
    int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    int ty = sh / 2 - 100;

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    while (std::getline(f, line)) {
        if (line.empty()) { ty += fontSize; continue; }
        Vector2 sz = MeasureTextEx(font, line.c_str(), (float)fontSize, 0);
        GuiLabel(rec(cx - sz.x / 2, (float)ty, sz.x, sz.y + 2), line.c_str());
        ty += (int)sz.y + 2;
    }

    int by = ty + 20;
    if (victory) {
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
        GuiLabel(rec(cx - 150, by, 300, 20), "Gracias por jugar Tenebrarium.");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
        GuiLabel(rec(cx - 150, by + 25, 300, 20), "~ Creditos ~");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_YELLOW));
        GuiLabel(rec(cx - 150, by + 50, 300, 20), "Halley & Nato Co.");
    }

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiLabel(rec(cx - 150, sh - 60, 300, 20), "ENTER para volver al menu");
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
}

// ─── Quit Dialog ─────────────────────────────────────────────────────────────

void RayguiGUI::drawQuitDialog(int selection) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = sh / 2;
    int bw = 240;
    int bh = 180;
    int bx = cx - bw / 2;
    int by = cy - bh / 2;

    // Semi-transparent background
    DrawRectangle(0, 0, sw, sh, {0, 0, 0, 180});

    // Dialog box
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(COL_YELLOW));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiLabel(rec(bx + 20, by + 15, bw - 40, 25), "Salir del juego");
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    GuiLabel(rec(bx + 20, by + 45, bw - 40, 20), "Que deseas hacer?");

    const char* opts[] = { "Menu Principal", "Salir al escritorio" };
    for (int i = 0; i < 2; i++) {
        int y = by + 80 + i * 35;
        if (selection == i) {
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_YELLOW));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_BLACK));
        } else {
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
        }
        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(COL_YELLOW));
        GuiButton(rec(bx + 30, y, bw - 60, 30), opts[i]);
    }

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
    GuiLabel(rec(bx + 20, by + bh - 25, bw - 40, 20), "ESC para cancelar");

    // Reset styles
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
}
