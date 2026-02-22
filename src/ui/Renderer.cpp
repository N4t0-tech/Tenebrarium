#include "Renderer.hpp"
#include <ncurses.h>
#include <string>
#include <array>
#include <vector>
#include <fstream>

// Loads a text file and returns its lines.
static std::vector<std::string> loadLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) lines.push_back(line);
    return lines;
}

// ─── helpers ────────────────────────────────────────────────────────────────

void Renderer::drawBox(int y, int x, int h, int w) {
    mvaddch(y,         x,         ACS_ULCORNER);
    mvaddch(y,         x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x,         ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    for (int i = 1; i < w - 1; i++) {
        mvaddch(y,         x + i, ACS_HLINE);
        mvaddch(y + h - 1, x + i, ACS_HLINE);
    }
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x,         ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
}

void Renderer::drawCentered(int row, const std::string& text) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows;
    int x = std::max(0, (cols - static_cast<int>(text.size())) / 2);
    mvprintw(row, x, "%s", text.c_str());
}

// ─── Title screen ───────────────────────────────────────────────────────────

void Renderer::drawTitle(int selection) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // Load and draw title art from file
    auto titleLines = loadLines(ASSETS_DIR "/title.txt");
    int titleH = static_cast<int>(titleLines.size());
    int titleStartY = rows / 2 - titleH / 2 - 4;

    attron(COLOR_PAIR(3) | A_BOLD);
    for (int i = 0; i < titleH; i++) {
        drawCentered(titleStartY + i, titleLines[i]);
    }
    attroff(COLOR_PAIR(3) | A_BOLD);

    // Subtitle
    int afterTitle = titleStartY + titleH + 1;
    attron(COLOR_PAIR(1));
    drawCentered(afterTitle, "~ Un RPG de mazmorra y sombras ~");
    attroff(COLOR_PAIR(1));

    // Options
    int optY = afterTitle + 3;
    int centerX = cols / 2;

    // Nueva Partida
    if (selection == 0) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    else                attron(COLOR_PAIR(1));
    std::string opt0 = "  Nueva Partida  ";
    mvprintw(optY, centerX - 20, "%s", opt0.c_str());
    attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE | COLOR_PAIR(1));

    // Salir
    if (selection == 1) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    else                attron(COLOR_PAIR(1));
    std::string opt1 = "     Salir     ";
    mvprintw(optY, centerX + 5, "%s", opt1.c_str());
    attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE | COLOR_PAIR(1));

    // Hint
    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows - 2, "Flechas para navegar  |  ENTER para confirmar");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Name input ─────────────────────────────────────────────────────────────

void Renderer::drawNameInput(const std::string& name) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    attron(COLOR_PAIR(3) | A_BOLD);
    drawCentered(rows / 2 - 5, "T E N E B R A R I U M");
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    drawCentered(rows / 2 - 2, "Ingresa el nombre de tu personaje:");
    attroff(COLOR_PAIR(1));

    // Input box
    int boxW = 24;
    int boxX = (cols - boxW) / 2;
    int boxY = rows / 2;
    drawBox(boxY, boxX, 3, boxW);

    // Name text + cursor
    std::string display = name + "_";
    mvprintw(boxY + 1, boxX + (boxW - static_cast<int>(display.size())) / 2,
             "%s", display.c_str());

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows / 2 + 4, "ENTER para continuar  |  ESC para volver");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Class select ────────────────────────────────────────────────────────────

struct ClassInfo {
    const char* name;
    const char* role;
    const char* description;
    int hp, atk, def, mana;
};

static constexpr std::array<ClassInfo, 3> kClasses = {{
    { "GUERRERO", "Vanguardia",
      "Maestro del combate cuerpo a cuerpo. Alto aguante y defensa.",
      120, 15, 8, 20 },
    { "MAGO",     "Arcano",
      "Domina las artes magicas. Baja defensa pero poder devastador.",
      70, 8, 3, 100 },
    { "RANGER",   "Explorador",
      "Agil y versatil. Experto en trampas, venenos y el arco.",
      90, 12, 5, 50 },
}};

void Renderer::drawClassSelect(int selection) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    attron(COLOR_PAIR(3) | A_BOLD);
    drawCentered(rows / 2 - 9, "T E N E B R A R I U M");
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    drawCentered(rows / 2 - 7, "Elige tu clase:");
    attroff(COLOR_PAIR(1));

    // Three class boxes
    const int boxH = 10;
    const int boxW = 18;
    const int totalW = boxW * 3 + 4; // 2 gaps of 2
    const int startX = (cols - totalW) / 2;
    const int startY = rows / 2 - 5;

    for (int i = 0; i < 3; i++) {
        int bx = startX + i * (boxW + 2);
        bool selected = (i == selection);

        if (selected) attron(COLOR_PAIR(5) | A_BOLD);
        else          attron(COLOR_PAIR(1));

        drawBox(startY, bx, boxH, boxW);

        // Class name centered inside box
        std::string className = kClasses[i].name;
        int nameX = bx + (boxW - static_cast<int>(className.size())) / 2;
        mvprintw(startY + 1, nameX, "%s", className.c_str());

        // Role
        std::string role = std::string("[ ") + kClasses[i].role + " ]";
        int roleX = bx + (boxW - static_cast<int>(role.size())) / 2;
        mvprintw(startY + 2, roleX, "%s", role.c_str());

        attroff(COLOR_PAIR(5) | A_BOLD | COLOR_PAIR(1));

        // Stats
        if (selected) attron(COLOR_PAIR(4) | A_BOLD);
        else          attron(COLOR_PAIR(4));

        mvprintw(startY + 4, bx + 3, "HP   : %3d", kClasses[i].hp);
        mvprintw(startY + 5, bx + 3, "ATK  : %3d", kClasses[i].atk);
        mvprintw(startY + 6, bx + 3, "DEF  : %3d", kClasses[i].def);
        mvprintw(startY + 7, bx + 3, "MANA : %3d", kClasses[i].mana);

        attroff(COLOR_PAIR(4) | A_BOLD);
    }

    // Description of selected class
    attron(COLOR_PAIR(1));
    drawCentered(startY + boxH + 1, kClasses[selection].description);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows - 2, "Flechas para navegar  |  ENTER para confirmar  |  ESC para volver");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Game screens (stubs) ───────────────────────────────────────────────────

void Renderer::drawExploration() {
    mvprintw(0, 0, "[Exploración] - en construcción  (Q para salir)");
}

void Renderer::drawCombat() {
    mvprintw(0, 0, "[Combate] - en construcción");
}

void Renderer::drawInventory() {
    mvprintw(0, 0, "[Inventario] - en construcción");
}

void Renderer::drawQuestLog() {
    mvprintw(0, 0, "[Misiones] - en construcción");
}

void Renderer::drawGameOver() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)cols;

    attron(COLOR_PAIR(2) | A_BOLD);
    drawCentered(rows / 2 - 1, "G A M E   O V E R");
    attroff(COLOR_PAIR(2) | A_BOLD);

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows / 2 + 1, "ENTER para volver al menú");
    attroff(COLOR_PAIR(1) | A_DIM);
}
