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
    mvprintw(y,         x,         "┌");
    mvprintw(y,         x + w - 1, "┐");
    mvprintw(y + h - 1, x,         "└");
    mvprintw(y + h - 1, x + w - 1, "┘");
    for (int i = 1; i < w - 1; i++) {
        mvprintw(y,         x + i, "─");
        mvprintw(y + h - 1, x + i, "─");
    }
    for (int i = 1; i < h - 1; i++) {
        mvprintw(y + i, x,         "│");
        mvprintw(y + i, x + w - 1, "│");
    }
}

void Renderer::drawCentered(int row, const std::string& text) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows;
    int x = std::max(0, (cols - static_cast<int>(text.size())) / 2);
    mvprintw(row, x, "%s", text.c_str());
}

// Draws a filled bar like [████░░░░] using Unicode block characters
void Renderer::drawStatBar(int y, int x, int w, int value, int max, int colorPair) {
    int fill = (max > 0) ? (value * w / max) : 0;
    attron(COLOR_PAIR(colorPair) | A_BOLD);
    for (int i = 0; i < w; i++)
        mvprintw(y, x + i, "%s", i < fill ? "█" : "░");
    attroff(COLOR_PAIR(colorPair) | A_BOLD);
}

// ─── Title screen ────────────────────────────────────────────────────────────

void Renderer::drawTitle(int selection) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    auto titleLines = loadLines(ASSETS_DIR "/title.txt");
    int titleH = static_cast<int>(titleLines.size());
    int titleStartY = rows / 2 - titleH / 2 - 4;

    attron(COLOR_PAIR(3) | A_BOLD);
    for (int i = 0; i < titleH; i++)
        drawCentered(titleStartY + i, titleLines[i]);
    attroff(COLOR_PAIR(3) | A_BOLD);

    int afterTitle = titleStartY + titleH + 1;
    attron(COLOR_PAIR(1));
    drawCentered(afterTitle, "~ Un RPG de mazmorra y sombras ~");
    attroff(COLOR_PAIR(1));

    int optY = afterTitle + 3;
    int centerX = cols / 2;

    if (selection == 0) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    else                attron(COLOR_PAIR(1));
    mvprintw(optY, centerX - 20, "  Nueva Partida  ");
    attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE | COLOR_PAIR(1));

    if (selection == 1) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    else                attron(COLOR_PAIR(1));
    mvprintw(optY, centerX + 5, "     Salir     ");
    attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE | COLOR_PAIR(1));

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows - 2, "Flechas para navegar  |  ENTER para confirmar");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Name input ──────────────────────────────────────────────────────────────

void Renderer::drawNameInput(const std::string& name) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    attron(COLOR_PAIR(3) | A_BOLD);
    drawCentered(rows / 2 - 5, "T E N E B R A R I U M");
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    drawCentered(rows / 2 - 2, "Ingresa el nombre de tu personaje:");
    attroff(COLOR_PAIR(1));

    int boxW = 24;
    int boxX = (cols - boxW) / 2;
    int boxY = rows / 2;
    drawBox(boxY, boxX, 3, boxW);

    std::string display = name + "_";
    mvprintw(boxY + 1, boxX + (boxW - static_cast<int>(display.size())) / 2,
             "%s", display.c_str());

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows / 2 + 4, "ENTER para continuar  |  ESC para volver");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Class select ─────────────────────────────────────────────────────────────

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

    const int boxH = 10, boxW = 18;
    const int totalW = boxW * 3 + 4;
    const int startX = (cols - totalW) / 2;
    const int startY = rows / 2 - 5;

    for (int i = 0; i < 3; i++) {
        int bx = startX + i * (boxW + 2);
        bool selected = (i == selection);

        if (selected) attron(COLOR_PAIR(5) | A_BOLD);
        else          attron(COLOR_PAIR(1));

        drawBox(startY, bx, boxH, boxW);
        std::string className = kClasses[i].name;
        mvprintw(startY + 1, bx + (boxW - static_cast<int>(className.size())) / 2,
                 "%s", className.c_str());
        std::string role = std::string("[ ") + kClasses[i].role + " ]";
        mvprintw(startY + 2, bx + (boxW - static_cast<int>(role.size())) / 2,
                 "%s", role.c_str());

        attroff(COLOR_PAIR(5) | A_BOLD | COLOR_PAIR(1));

        if (selected) attron(COLOR_PAIR(4) | A_BOLD);
        else          attron(COLOR_PAIR(4));

        mvprintw(startY + 4, bx + 3, "HP   : %3d", kClasses[i].hp);
        mvprintw(startY + 5, bx + 3, "ATK  : %3d", kClasses[i].atk);
        mvprintw(startY + 6, bx + 3, "DEF  : %3d", kClasses[i].def);
        mvprintw(startY + 7, bx + 3, "MANA : %3d", kClasses[i].mana);

        attroff(COLOR_PAIR(4) | A_BOLD);
    }

    attron(COLOR_PAIR(1));
    drawCentered(startY + boxH + 1, kClasses[selection].description);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows - 2, "Flechas para navegar  |  ENTER para confirmar  |  ESC para volver");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── HUD select ──────────────────────────────────────────────────────────────

void Renderer::drawHudSelect(int selection) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    attron(COLOR_PAIR(3) | A_BOLD);
    drawCentered(rows / 2 - 10, "T E N E B R A R I U M");
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    drawCentered(rows / 2 - 8, "Elige el estilo de interfaz:");
    attroff(COLOR_PAIR(1));

    // Two preview boxes side by side
    const int previewW = 30;
    const int previewH = 12;
    const int gap      = 6;
    const int totalW   = previewW * 2 + gap;
    const int startX   = (cols - totalW) / 2;
    const int startY   = rows / 2 - 5;

    // ── Sidebar preview ──
    {
        int bx = startX;
        bool sel = (selection == 0);
        if (sel) attron(COLOR_PAIR(2) | A_BOLD);
        else     attron(COLOR_PAIR(1) | A_DIM);

        drawBox(startY, bx, previewH, previewW);

        // Map area
        for (int r = 1; r < previewH - 1; r++)
            for (int c = 1; c < previewW - 7; c++)
                mvprintw(startY + r, bx + c, "·");

        // Sidebar strip
        mvprintw(startY,              bx + previewW - 7, "┬");
        mvprintw(startY + previewH-1, bx + previewW - 7, "┴");
        for (int r = 1; r < previewH - 1; r++)
            mvprintw(startY + r, bx + previewW - 7, "│");

        mvprintw(startY + 2,  bx + previewW - 6, "Hero");
        mvprintw(startY + 3,  bx + previewW - 6, "Lv.1");
        mvprintw(startY + 5,  bx + previewW - 6, "HP██");
        mvprintw(startY + 6,  bx + previewW - 6, "MP░░");
        mvprintw(startY + 8,  bx + previewW - 6, "EQP.");

        // Player
        attron(A_BOLD);
        mvprintw(startY + previewH / 2, bx + (previewW - 7) / 2, "@");
        attroff(A_BOLD);

        attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1) | A_DIM);

        if (sel) attron(COLOR_PAIR(2) | A_BOLD);
        else     attron(COLOR_PAIR(1));
        drawCentered(startY + previewH + 1, "Panel lateral");
        attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1));
    }

    // ── Bottom bar preview ──
    {
        int bx = startX + previewW + gap;
        bool sel = (selection == 1);
        if (sel) attron(COLOR_PAIR(2) | A_BOLD);
        else     attron(COLOR_PAIR(1) | A_DIM);

        drawBox(startY, bx, previewH, previewW);

        // Map area
        for (int r = 1; r < previewH - 4; r++)
            for (int c = 1; c < previewW - 1; c++)
                mvprintw(startY + r, bx + c, "·");

        // Bottom bar separator
        int barY = startY + previewH - 4;
        mvprintw(barY, bx,              "├");
        mvprintw(barY, bx + previewW-1, "┤");
        for (int c = 1; c < previewW - 1; c++)
            mvprintw(barY, bx + c, "─");

        mvprintw(barY + 1, bx + 1, "Hero Lv1 HP██░  MP░░");
        mvprintw(barY + 2, bx + 1, "EQP: - Arma: -  DEF:");

        // Player
        attron(A_BOLD);
        mvprintw(startY + (previewH - 4) / 2, bx + previewW / 2, "@");
        attroff(A_BOLD);

        attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1) | A_DIM);

        if (sel) attron(COLOR_PAIR(2) | A_BOLD);
        else     attron(COLOR_PAIR(1));
        drawCentered(startY + previewH + 1, "Barra inferior");
        attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1));
    }

    attron(COLOR_PAIR(1) | A_DIM);
    drawCentered(rows - 2, "Flechas para navegar  |  ENTER para confirmar  |  ESC para volver");
    attroff(COLOR_PAIR(1) | A_DIM);
}

// ─── Exploration ─────────────────────────────────────────────────────────────

void Renderer::drawExploration(const Map& map, const Player& player, HudLayout layout) {
    if (layout == HudLayout::Sidebar)
        drawExplorationSidebar(map, player);
    else
        drawExplorationBottom(map, player);
}

static const char* className(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior: return "Guerrero";
        case PlayerClass::Mage:    return "Mago";
        case PlayerClass::Ranger:  return "Ranger";
    }
    return "";
}

void Renderer::drawHudPanel(const Player& player, int startX, int w, int h) {
    // Vertical panel drawn at startX, full screen height h
    drawBox(0, startX, h, w);

    int y = 1;
    // Name
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y++, startX + 2, "%.14s", player.getName().c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    mvprintw(y++, startX + 2, "[%s]", className(player));
    mvprintw(y++, startX + 2, "Nivel %d", player.getLevel());
    attroff(COLOR_PAIR(1));

    // Separator
    mvprintw(y, startX,         "├");
    mvprintw(y, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(y, startX + c, "─");
    y++;

    // HP
    attron(COLOR_PAIR(4));
    mvprintw(y++, startX + 2, "HP  %3d/%3d", player.getHp(), player.getMaxHp());
    attroff(COLOR_PAIR(4));
    drawStatBar(y++, startX + 2, w - 4, player.getHp(), player.getMaxHp(), 4);

    // Mana
    attron(COLOR_PAIR(3));
    mvprintw(y++, startX + 2, "MP  %3d/%3d", player.getMana(), player.getMaxMana());
    attroff(COLOR_PAIR(3));
    drawStatBar(y++, startX + 2, w - 4, player.getMana(), player.getMaxMana(), 3);

    // XP
    attron(COLOR_PAIR(2));
    mvprintw(y++, startX + 2, "XP  %d", player.getXp());
    attroff(COLOR_PAIR(2));

    // Separator
    mvprintw(y, startX,         "├");
    mvprintw(y, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(y, startX + c, "─");
    y++;

    // Stats
    attron(COLOR_PAIR(1));
    mvprintw(y++, startX + 2, "ATK  %d", player.getAttack());
    mvprintw(y++, startX + 2, "DEF  %d", player.getDefense());
    attroff(COLOR_PAIR(1));

    // Separator
    mvprintw(y, startX,         "├");
    mvprintw(y, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(y, startX + c, "─");
    y++;

    // Equipment
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(y++, startX + 2, "EQUIPO");
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(y++, startX + 2, "Arma    : -");
    mvprintw(y++, startX + 2, "Armadura: -");
    mvprintw(y++, startX + 2, "Accesorio: -");
    attroff(COLOR_PAIR(1) | A_DIM);

    // Separator
    mvprintw(y, startX,         "├");
    mvprintw(y, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(y, startX + c, "─");
    y++;

    // Inventory slots
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(y++, startX + 2, "Inventario");
    attroff(COLOR_PAIR(1) | A_BOLD);
    mvprintw(y++, startX + 2, "%d/%d slots",
             player.getInventory().usedSlots(),
             player.getInventory().totalSlots());
    drawStatBar(y++, startX + 2, w - 4,
                player.getInventory().usedSlots(),
                player.getInventory().totalSlots(), 1);

    // Controls hint at bottom
    mvprintw(h - 3, startX,         "├");
    mvprintw(h - 3, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(h - 3, startX + c, "─");
    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(h - 2, startX + 2, "WASD  mover");
    mvprintw(h - 1, startX + 2, "  Q   salir");
    attroff(COLOR_PAIR(1) | A_DIM);
}

void Renderer::drawHudBar(const Player& player, int startY, int startX, int w) {
    // Horizontal bar
    mvprintw(startY, startX,         "├");
    mvprintw(startY, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(startY, startX + c, "─");

    int y = startY + 1;

    // Row 1: name, class, level, HP, mana
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, startX + 1, "%.12s", player.getName().c_str());
    attroff(COLOR_PAIR(3) | A_BOLD);

    attron(COLOR_PAIR(1));
    mvprintw(y, startX + 14, "[%s] Nv.%d", className(player), player.getLevel());
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(y, startX + 34, "HP:%d/%d", player.getHp(), player.getMaxHp());
    attroff(COLOR_PAIR(4) | A_BOLD);
    drawStatBar(y, startX + 47, 12, player.getHp(), player.getMaxHp(), 4);

    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, startX + 61, "MP:%d/%d", player.getMana(), player.getMaxMana());
    attroff(COLOR_PAIR(3) | A_BOLD);
    y++;

    // Row 2: equipment, stats
    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(y, startX + 1, "ATK:%-3d DEF:%-3d  Arma:%-10s Armadura:%-10s",
             player.getAttack(), player.getDefense(), "-", "-");
    attroff(COLOR_PAIR(1) | A_DIM);

    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(y, startX + w - 22, "Inv:%d/%d  WASD:mover  Q:salir",
             player.getInventory().usedSlots(),
             player.getInventory().totalSlots());
    attroff(COLOR_PAIR(1) | A_DIM);
}

void Renderer::drawMap(const Map& map, const Player& player,
                       int viewX, int viewY, int viewW, int viewH) {
    Position playerPos = map.getPlayerPos();
    int camX = playerPos.x - viewW / 2;
    int camY = playerPos.y - viewH / 2;

    for (int sy = 0; sy < viewH; sy++) {
        for (int sx = 0; sx < viewW; sx++) {
            int mx = camX + sx;
            int my = camY + sy;

            int screenX = viewX + sx;
            int screenY = viewY + sy;

            if (mx < 0 || mx >= map.width() || my < 0 || my >= map.height()) {
                mvaddch(screenY, screenX, ' ');
                continue;
            }

            const Tile& tile = map.at(mx, my);
            if (!tile.explored) { mvaddch(screenY, screenX, ' '); continue; }

            if (mx == playerPos.x && my == playerPos.y) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddch(screenY, screenX, '@');
                attroff(COLOR_PAIR(2) | A_BOLD);
                continue;
            }

            if (tile.visible) {
                attron(COLOR_PAIR(1));
                mvaddch(screenY, screenX, tile.glyph);
                attroff(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(1) | A_DIM);
                mvaddch(screenY, screenX, tile.glyph);
                attroff(COLOR_PAIR(1) | A_DIM);
            }
        }
    }
    (void)player;
}

void Renderer::drawExplorationSidebar(const Map& map, const Player& player) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int panelW = 22;
    const int mapW   = cols - panelW;

    drawMap(map, player, 0, 0, mapW, rows);
    drawHudPanel(player, mapW, panelW, rows);
}

void Renderer::drawExplorationBottom(const Map& map, const Player& player) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int barRows = 4;   // separator + 2 content rows + bottom border
    const int mapH    = rows - barRows;

    drawBox(0, 0, rows, cols);
    drawMap(map, player, 1, 1, cols - 2, mapH - 1);
    drawHudBar(player, mapH, 0, cols);
}

// ─── Game screens (stubs) ────────────────────────────────────────────────────

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
