#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "RayguiGUI.hpp"
#include "ui/XpLoader.hpp"
#include "core/Assets.hpp"
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

// ─── Init ────────────────────────────────────────────────────────────────────

static int gBaseFontSize = 18;

void RayguiGUI::init(Font font, int fontSize) {
    gBaseFontSize = fontSize;
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
    GuiSetStyle(DEFAULT, TEXT_SIZE, gBaseFontSize);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(COL_GRAY));
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
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
    GuiLabel(rec(cx - 150, cy - 130, 300, 25), "T E N E B R A R I U M");

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 100, cy - 95, 200, 25), "Configuracion");

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

    int rowH = 36;
    int startY = cy - 50;

    for (int i = 0; i < 4; i++) {
        int y = startY + i * rowH;
        bool sel = (i == selection);

        if (sel) {
            DrawRectangle(cx - 160, y, 410, rowH, {255, 230, 60, 25});
        }

        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(sel ? COL_YELLOW : COL_WHITE));
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiLabel(rec(cx - 150, y, 260, rowH), items[i]);

        if (vals[i][0] != '\0') {
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(sel ? COL_CYAN : COL_GRAY));
            GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
            GuiLabel(rec(cx + 115, y, 135, rowH), vals[i]);
        }
    }

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiLabel(rec(cx - 250, startY + 4 * rowH + 12, 500, 25),
             "W/S navegar  |  ENTER cambiar  |  ESC volver");
}

// ─── Game Over ───────────────────────────────────────────────────────────────

void RayguiGUI::drawGameOver(bool victory) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    Font font = GuiGetFont();
    int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    Color artColor = victory ? COL_YELLOW : COL_RED;

    std::ifstream f(assetsDir() + (victory ? "victoryTitle.txt" : "gameOverTitle.txt"));
    std::string line;
    int ty = sh / 2 - 80;

    while (std::getline(f, line)) {
        if (line.empty()) { ty += fontSize; continue; }
        Vector2 sz = MeasureTextEx(font, line.c_str(), (float)fontSize, 0);
        DrawTextEx(font, line.c_str(), {(float)(cx - sz.x / 2), (float)ty}, (float)fontSize, 0, artColor);
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
    GuiLabel(rec(cx - 250, sh - 60, 500, 20), "ENTER para volver al menu");
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

// ─── Title ───────────────────────────────────────────────────────────────────

// Returns Y position after the last line of ASCII art
static int drawAsciiTitle() {
    int sw = GetScreenWidth();
    int cx = sw / 2;
    Font font = GuiGetFont();
    int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    std::ifstream f(assetsDir() + "title.txt");
    std::string line;
    int ty = GetScreenHeight() / 2 - 120;

    while (std::getline(f, line)) {
        if (line.empty()) { ty += fontSize; continue; }
        Vector2 sz = MeasureTextEx(font, line.c_str(), (float)fontSize, 0);
        DrawTextEx(font, line.c_str(), {(float)(cx - sz.x / 2), (float)ty}, (float)fontSize, 0, COL_GRAY);
        ty += (int)sz.y + 2;
    }
    return ty;
}

void RayguiGUI::drawTitle(int selection, bool hasSave, bool blink) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    int ty = drawAsciiTitle();
    ty += GuiGetStyle(DEFAULT, TEXT_SIZE);

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 150, ty, 300, 20), "~ Un RPG de mazmorra y sombras ~");
    ty += 30;

    const char* opts4[] = { "Continuar", "Nueva Partida", "Configuracion", "Creditos", "Salir" };
    const char* opts3[] = { "Nueva Partida", "Configuracion", "Creditos", "Salir" };
    int n = hasSave ? 5 : 4;
    const char** opts = hasSave ? opts4 : opts3;

    for (int i = 0; i < n; i++) {
        if (selection == i) {
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_YELLOW));
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, blink ? ColorToInt({255,230,60,80}) : ColorToInt(COL_BLACK));
        } else {
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
        }
        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiButton(rec(cx - 100, ty, 200, 28), opts[i]);
        ty += 32;
    }

    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiLabel(rec(cx - 250, ty + 10, 500, 20), "W/S navegar  |  ENTER para confirmar");
}

// ─── Credits ─────────────────────────────────────────────────────────────────

void RayguiGUI::drawCredits() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    int ty = drawAsciiTitle();
    ty += GuiGetStyle(DEFAULT, TEXT_SIZE);

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 100, ty, 200, 20), "~ Creditos ~");
    ty += 25;
    GuiLabel(rec(cx - 100, ty, 200, 20), "Halley & Nato Co.");
    ty += 35;
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiLabel(rec(cx - 100, ty, 200, 20), "ESC para volver");
}

// ─── Name Input ──────────────────────────────────────────────────────────────

void RayguiGUI::drawNameInput(const std::string& name, bool blink) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    int ty = drawAsciiTitle();
    ty += GuiGetStyle(DEFAULT, TEXT_SIZE);

    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 150, ty, 300, 20), "~ Un RPG de mazmorra y sombras ~");
    ty += 30;
    GuiLabel(rec(cx - 150, ty, 300, 20), "Ingresa el nombre de tu personaje:");
    ty += 30;

    std::string display = "> " + name + (blink ? "_" : " ");
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_YELLOW));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiLabel(rec(cx - 200, ty, 400, 25), display.c_str());

    ty += 40;
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiLabel(rec(cx - 250, ty, 500, 20), "ENTER para continuar  |  ESC para volver");
}

// ─── Class Select ────────────────────────────────────────────────────────────

static constexpr int kClassHp[3]  = {120, 70, 90};
static constexpr int kClassAtk[3] = {20, 8, 12};
static constexpr int kClassDef[3] = {8, 3, 5};
static constexpr int kClassMana[3]= {100, 100, 50};
static const char* kClassName[3]  = {"GUERRERO", "MAGO", "RANGER"};
static const char* kClassRole[3]  = {"Vanguardia", "Arcano", "Explorador"};
static const char* kClassDesc[3]  = {
    "Maestro del combate cuerpo a cuerpo.",
    "Domina las artes magicas. Poder devastador.",
    "Agil y versatil. Experto en trampas y arco.",
};

void RayguiGUI::drawClassSelect(int selection) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    GuiLabel(rec(cx - 150, 15, 300, 30), "T E N E B R A R I U M");
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 150, 45, 300, 25), "Elige tu clase:");

    int boxY = 80;
    int boxW = 260;
    int boxH = 140;
    int spacing = 20;

    for (int i = 0; i < 3; i++) {
        int bx = cx - boxW / 2;
        int by = boxY + i * (boxH + spacing);
        bool sel = (i == selection);

        Color borderCol = sel ? COL_YELLOW : COL_GRAY;
        Color textCol = sel ? COL_YELLOW : COL_GRAY;
        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(borderCol));
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(textCol));

        if (sel) {
            GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
        } else {
            GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
        }

        GuiPanel(rec(bx, by, boxW, boxH), "");

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(textCol));
        GuiLabel(rec(bx + 10, by + 8, boxW - 20, 20), kClassName[i]);

        GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
        std::string roleLine = std::string("[ ") + kClassRole[i] + " ]";
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(sel ? COL_YELLOW : COL_GRAY));
        GuiLabel(rec(bx + 10, by + 30, boxW - 20, 18), roleLine.c_str());

        std::string manaLabel = (i == 0) ? " AG:" : " MP:";
        std::string stats = "HP:" + std::to_string(kClassHp[i]) +
                            " ATK:" + std::to_string(kClassAtk[i]) +
                            " DEF:" + std::to_string(kClassDef[i]) +
                            manaLabel + std::to_string(kClassMana[i]);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GREEN));
        GuiLabel(rec(bx + 10, by + 55, boxW - 20, 18), stats.c_str());

        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(sel ? COL_WHITE : COL_GRAY));
        GuiLabel(rec(bx + 10, by + 78, boxW - 20, 18), kClassDesc[i]);

        if (sel) {
            GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt({255,230,60,30}));
            GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
        }
    }

    // Retrato de la clase seleccionada (al lado derecho de los paneles)
    {
        const char* portraitFiles[3] = { "warrior.xp", "mago.xp", "ranger.xp" };
        int px = cx + boxW / 2 + 30;
        int py = boxY + boxH / 2;
        try {
            XpFile xp = loadXp(assetsDir() + "art/" + portraitFiles[selection]);
            if (!xp.layers.empty()) {
                float cellW = 4.0f;
                float cellH = 8.0f;
                int ph = (xp.layers[0].height / 2) * cellH;
                xpDrawHalfBlockRl(px, py - ph / 2, xp.layers[0], cellW, cellH);
            }
        } catch (...) {
            GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
            GuiLabel(rec(px, py - 10, 100, 20), "[retrato N/A]");
        }
    }

    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiLabel(rec(cx - 250, boxY + 3 * (boxH + spacing) + 5, 500, 20),
             "W/S navegar  |  ENTER confirmar  |  ESC volver");
}

// ─── HUD Select ──────────────────────────────────────────────────────────────

void RayguiGUI::drawHudSelect(int selection) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;
    int cy = sh / 2;

    DrawRectangle(0, 0, sw, sh, COL_BLACK);

    Font hudFont = GuiGetFont();
    int hudFontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);

    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 22);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
    GuiLabel(rec(cx - 150, cy - 130, 300, 30), "T E N E B R A R I U M");
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
    GuiLabel(rec(cx - 150, cy - 100, 300, 25), "Elige el estilo de interfaz:");

    // Sidebar preview
    int previewW = 220;
    int previewH = 140;
    int previewY = cy - 65;
    int leftX = cx - previewW - 25;
    int rightX = cx + 25;

    // ── Sidebar (izquierda) ──
    {
        int px = leftX;
        int py = previewY;
        bool sel = (selection == 0);
        Color bc = sel ? COL_YELLOW : COL_GRAY;

        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(bc));
        GuiSetStyle(DEFAULT, BORDER_WIDTH, sel ? 2 : 1);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
        GuiPanel(rec(px, py, previewW, previewH), "");

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        for (int r = 0; r < 7; r++) {
            std::string mm = (r == 3) ? ". . . .@. . . ." : ". . . . . . . . .";
            DrawTextEx(hudFont, mm.c_str(), {(float)(px + 8), (float)(py + 8 + r * 11)}, (float)hudFontSize, 0, COL_GRAY);
        }

        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(bc));
        GuiPanel(rec(px + previewW - 55, py + 5, 50, previewH - 10), "");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
        GuiLabel(rec(px + previewW - 48, py + 10, 40, 12), "Hero");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
        GuiLabel(rec(px + previewW - 48, py + 22, 40, 12), "Lv.1");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GREEN));
        GuiLabel(rec(px + previewW - 48, py + 45, 40, 12), "HP##");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_CYAN));
        GuiLabel(rec(px + previewW - 48, py + 57, 40, 12), "MP..");

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(bc));
        GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
        GuiLabel(rec(px, py + previewH + 5, previewW, 18), "Panel lateral");
    }

    // ── Bottom (derecha) ──
    {
        int px = rightX;
        int py = previewY;
        bool sel = (selection == 1);
        Color bc = sel ? COL_YELLOW : COL_GRAY;

        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(bc));
        GuiSetStyle(DEFAULT, BORDER_WIDTH, sel ? 2 : 1);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
        GuiPanel(rec(px, py, previewW, previewH), "");

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        for (int r = 0; r < 6; r++) {
            std::string mm = (r == 2) ? ". . . . .@. . . ." : ". . . . . . . . .";
            DrawTextEx(hudFont, mm.c_str(), {(float)(px + 8), (float)(py + 5 + r * 11)}, (float)hudFontSize, 0, COL_GRAY);
        }

        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(bc));
        GuiPanel(rec(px + 5, py + previewH - 22, previewW - 10, 17), "");
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_WHITE));
        GuiSetStyle(DEFAULT, TEXT_SIZE, 8);
        GuiLabel(rec(px + 8, py + previewH - 20, previewW - 16, 14),
                 "Hero Lv1 HP##.. MP..");

        GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(bc));
        GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
        GuiLabel(rec(px, py + previewH + 5, previewW, 18), "Barra inferior");
    }

    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(COL_BLACK));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(COL_GRAY));
    GuiSetStyle(DEFAULT, TEXT_SIZE, 14);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiLabel(rec(cx - 250, previewY + previewH + 30, 500, 20),
             "A/D navegar  |  ENTER confirmar  |  ESC volver");
}
