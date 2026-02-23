#include "Renderer.hpp"
#include <ncurses.h>
#include <string>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>

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

void Renderer::drawExploration(const Map& map, const Player& player, HudLayout layout,
                               const std::vector<MapEntity>& entities,
                               const std::string& message) {
    if (layout == HudLayout::Sidebar)
        drawExplorationSidebar(map, player, entities, message);
    else
        drawExplorationBottom(map, player, entities, message);
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

    // Separator
    mvprintw(y, startX,         "├");
    mvprintw(y, startX + w - 1, "┤");
    for (int c = 1; c < w - 1; c++) mvprintw(y, startX + c, "─");
    y++;

    // Coins, keys, floor
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(y++, startX + 2, "$ %d monedas", player.getCoins());
    attroff(COLOR_PAIR(2) | A_BOLD);
    attron(COLOR_PAIR(3));
    mvprintw(y++, startX + 2, "k %d llave(s)", player.getKeys());
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(y++, startX + 2, "Piso %d", player.getDungeonFloor());
    attroff(COLOR_PAIR(1) | A_DIM);

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

    attron(COLOR_PAIR(2));
    mvprintw(y, startX + 52, "$%-3d", player.getCoins());
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    mvprintw(y, startX + 58, "k%d", player.getKeys());
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(1) | A_DIM);
    mvprintw(y, startX + 62, "P%d  WASD:mover  Q:salir", player.getDungeonFloor());
    attroff(COLOR_PAIR(1) | A_DIM);
}

void Renderer::drawMap(const Map& map, const Player& player,
                       int viewX, int viewY, int viewW, int viewH,
                       const std::vector<MapEntity>& entities) {
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

            // Draw map entities (only if tile is currently visible)
            if (tile.visible) {
                bool drew = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        int attr = COLOR_PAIR(ent.colorPair) | (ent.bold ? A_BOLD : A_DIM);
                        attron(attr);
                        mvaddch(screenY, screenX, ent.glyph);
                        attroff(attr);
                        drew = true;
                        break;
                    }
                }
                if (drew) continue;
            }

            // Secret walls: mid-grey '#' (test highlight)
            if (tile.type == TileType::SecretWall) {
                int attr = COLOR_PAIR(7) | (COLORS < 256 ? A_DIM : 0);
                attron(attr);
                mvaddch(screenY, screenX, '#');
                attroff(attr);
            } else if (tile.visible) {
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

void Renderer::drawExplorationSidebar(const Map& map, const Player& player,
                                       const std::vector<MapEntity>& entities,
                                       const std::string& message) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int panelW = 22;
    const int mapW   = cols - panelW;

    drawMap(map, player, 0, 0, mapW, rows, entities);
    drawHudPanel(player, mapW, panelW, rows);

    if (!message.empty()) {
        int msgW = std::min(static_cast<int>(message.size()) + 4, mapW - 2);
        int msgX = (mapW - msgW) / 2;
        attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvprintw(rows - 2, msgX, " %.*s ", msgW - 2, message.c_str());
        attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    }
}

void Renderer::drawExplorationBottom(const Map& map, const Player& player,
                                      const std::vector<MapEntity>& entities,
                                      const std::string& message) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const int barRows = 4;
    const int mapH    = rows - barRows;

    drawBox(0, 0, rows, cols);
    drawMap(map, player, 1, 1, cols - 2, mapH - 1, entities);
    drawHudBar(player, mapH, 0, cols);

    if (!message.empty()) {
        int msgW = std::min(static_cast<int>(message.size()) + 4, cols - 2);
        int msgX = (cols - msgW) / 2;
        attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
        mvprintw(mapH - 2, msgX, " %.*s ", msgW - 2, message.c_str());
        attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    }
}

// ─── Combat screen ───────────────────────────────────────────────────────────

static const char* classNameCombat(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior: return "Guerrero";
        case PlayerClass::Mage:    return "Mago";
        case PlayerClass::Ranger:  return "Ranger";
    }
    return "";
}

void Renderer::drawCombat(const CombatSystem& combat, const Player& player,
                           bool showingArts, int artSelection) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // ── layout constants ─────────────────────────────────────────────────────
    const int actionRows = showingArts ? 6 : 8; // arts submenu or action menu
    const int playerBarH = 3;  // player stats bar
    const int topH       = rows - playerBarH - actionRows;  // enemies + log area
    const int splitX     = cols / 2;  // vertical split between enemies and log

    // ── outer box ────────────────────────────────────────────────────────────
    drawBox(0, 0, rows, cols);

    // ── title bar ────────────────────────────────────────────────────────────
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(0, 2, " COMBATE ");
    attroff(COLOR_PAIR(3) | A_BOLD);

    // ── horizontal separator under title (row 1) ─────────────────────────────
    mvprintw(1, 0,         "├");
    mvprintw(1, cols - 1,  "┤");
    for (int c = 1; c < cols - 1; c++) mvprintw(1, c, "─");

    // ── vertical split in top area ────────────────────────────────────────────
    mvprintw(1,     splitX, "┬");
    mvprintw(topH,  splitX, "┴");
    for (int r = 2; r < topH; r++) mvprintw(r, splitX, "│");

    // ── column headers ────────────────────────────────────────────────────────
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(2, 2,           "ENEMIGOS");
    mvprintw(2, splitX + 2,  "LOG");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // ── enemies list ──────────────────────────────────────────────────────────
    int ey = 3;
    const auto& enemies = combat.getEnemies();
    for (int i = 0; i < static_cast<int>(enemies.size()); i++) {
        if (ey >= topH - 1) break;
        const auto& e = enemies[i];
        bool isTarget = (i == combat.getCurrentTarget());

        if (!e->isAlive()) {
            attron(COLOR_PAIR(1) | A_DIM);
            mvprintw(ey++, 3, "  %s [MUERTO]", e->getName().c_str());
            attroff(COLOR_PAIR(1) | A_DIM);
            continue;
        }

        if (isTarget) attron(COLOR_PAIR(2) | A_BOLD);
        else          attron(COLOR_PAIR(1));
        mvprintw(ey++, 3, "%s %s", isTarget ? ">" : " ", e->getName().c_str());
        attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1));

        if (ey < topH - 1) {
            // "  HP NNN/NNN [bar]" — numbers first so they never overflow into log
            char hpbuf[24];
            snprintf(hpbuf, sizeof(hpbuf), "  HP %3d/%-3d ", e->getHp(), e->getMaxHp());
            attron(COLOR_PAIR(4));
            mvprintw(ey, 3, "%s", hpbuf);
            attroff(COLOR_PAIR(4));
            int textEnd = 3 + static_cast<int>(strlen(hpbuf));
            int barW = splitX - textEnd - 2;  // leave 2 before the divider
            if (barW > 0)
                drawStatBar(ey, textEnd, barW, e->getHp(), e->getMaxHp(), 4);
            ey++;
        }

        // Show status effects (poison, frozen, etc.)
        const auto& efx = combat.getEnemyEffects(i);
        if (!efx.empty() && ey < topH - 1) {
            attron(COLOR_PAIR(2) | A_DIM);
            std::string tags;
            for (const auto& fx : efx) {
                switch (fx.type) {
                    case StatusEffect::Type::Poisoned:    tags += "[VEN]"; break;
                    case StatusEffect::Type::Frozen:      tags += "[HIE]"; break;
                    case StatusEffect::Type::TrapPending: tags += "[TRP]"; break;
                    default: break;
                }
            }
            if (!tags.empty()) mvprintw(ey++, 5, "%s", tags.c_str());
            attroff(COLOR_PAIR(2) | A_DIM);
        }

        ey++; // blank line between enemies
    }

    // ── log panel ─────────────────────────────────────────────────────────────
    {
        const auto& log = combat.getLog();
        const int colW  = cols - splitX - 3;  // usable width inside the log panel

        // Pre-wrap every log entry into screen lines (newest entries last)
        std::vector<std::string> wrapped;
        for (const auto& entry : log) {
            if (static_cast<int>(entry.size()) <= colW) {
                wrapped.push_back(entry);
            } else {
                // Hard-wrap at colW characters
                int pos = 0;
                while (pos < static_cast<int>(entry.size())) {
                    wrapped.push_back(entry.substr(pos, colW));
                    pos += colW;
                }
            }
        }

        // Show only the last N lines that fit in the panel
        int logAreaH = topH - 3;
        int start = std::max(0, static_cast<int>(wrapped.size()) - logAreaH);
        int ly = 3;
        for (int i = start; i < static_cast<int>(wrapped.size()) && ly < topH; i++, ly++) {
            attron(COLOR_PAIR(1));
            mvprintw(ly, splitX + 2, "%s", wrapped[i].c_str());
            attroff(COLOR_PAIR(1));
        }
    }

    // ── separator above player bar ────────────────────────────────────────────
    mvprintw(topH, 0,         "├");
    mvprintw(topH, cols - 1,  "┤");
    for (int c = 1; c < cols - 1; c++) mvprintw(topH, c, "─");

    // ── player stats bar ──────────────────────────────────────────────────────
    {
        int py = topH + 1;
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(py, 2, "%s [%s]", player.getName().c_str(), classNameCombat(player));
        attroff(COLOR_PAIR(3) | A_BOLD);

        // HP bar
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(py, 24, "HP:");
        attroff(COLOR_PAIR(4) | A_BOLD);
        drawStatBar(py, 27, 10, player.getHp(), player.getMaxHp(), 4);
        attron(COLOR_PAIR(4));
        mvprintw(py, 38, "%d/%d", player.getHp(), player.getMaxHp());
        attroff(COLOR_PAIR(4));

        // MP bar
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(py, 50, "MP:");
        attroff(COLOR_PAIR(3) | A_BOLD);
        drawStatBar(py, 53, 8, player.getMana(), player.getMaxMana(), 3);
        attron(COLOR_PAIR(3));
        mvprintw(py, 62, "%d/%d", player.getMana(), player.getMaxMana());
        attroff(COLOR_PAIR(3));

        // PA display
        py++;
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(py, 2, "PA:");
        attroff(COLOR_PAIR(1) | A_BOLD);
        int ap    = combat.getCurrentAp();
        int maxAp = combat.getMaxAp();
        for (int i = 0; i < maxAp; i++) {
            if (i < ap) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(py, 6 + i * 2, "●");
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1) | A_DIM);
                mvprintw(py, 6 + i * 2, "○");
                attroff(COLOR_PAIR(1) | A_DIM);
            }
        }
        attron(COLOR_PAIR(1) | A_DIM);
        mvprintw(py, 6 + maxAp * 2 + 1, "(%d/%d PA)", ap, maxAp);
        attroff(COLOR_PAIR(1) | A_DIM);

        // Player status effects
        const auto& pfx = combat.getPlayerEffects();
        if (!pfx.empty()) {
            std::string ptags;
            for (const auto& fx : pfx) {
                switch (fx.type) {
                    case StatusEffect::Type::Defending:      ptags += "[DEF]"; break;
                    case StatusEffect::Type::DefendingHeavy: ptags += "[ESC]"; break;
                    case StatusEffect::Type::AttackBoosted:  ptags += "[ATK+]"; break;
                    default: break;
                }
            }
            attron(COLOR_PAIR(2));
            mvprintw(py, cols - static_cast<int>(ptags.size()) - 3, "%s", ptags.c_str());
            attroff(COLOR_PAIR(2));
        }
    }

    // ── separator above actions ───────────────────────────────────────────────
    int actY = topH + playerBarH;
    mvprintw(actY, 0,         "├");
    mvprintw(actY, cols - 1,  "┤");
    for (int c = 1; c < cols - 1; c++) mvprintw(actY, c, "─");

    // ── action menu or arts submenu ───────────────────────────────────────────
    int ap = combat.getCurrentAp();
    actY++;

    if (!showingArts) {
        // Normal action menu
        auto printAction = [&](int row, int col, const char* key, const char* desc,
                                const char* cost, int required) {
            bool dim = (required > 0 && ap < required);
            if (dim) attron(COLOR_PAIR(1) | A_DIM);
            else     attron(COLOR_PAIR(1));
            mvprintw(row, col, "%s", key);
            attroff(COLOR_PAIR(1) | A_DIM | COLOR_PAIR(1));

            if (dim) attron(A_DIM);
            mvprintw(row, col + static_cast<int>(std::string(key).size()), " %-18s%s", desc, cost);
            if (dim) attroff(A_DIM);
        };

        int col = 2;
        int col2 = cols / 2 + 1;
        printAction(actY,     col,  "[A]",     "Atacar",         "1 PA",  1);
        printAction(actY,     col2, "[F]",     "Ataque Fuerte",  "2 PA",  2);
        actY++;
        {
            auto arts = player.getAvailableArts();
            int minCost = 99, maxCost = 0;
            for (const auto& a : arts) {
                if (a.apCost < minCost) minCost = a.apCost;
                if (a.apCost > maxCost) maxCost = a.apCost;
            }
            char artCost[12];
            if (minCost == maxCost)
                snprintf(artCost, sizeof(artCost), "%d PA", minCost);
            else
                snprintf(artCost, sizeof(artCost), "%d-%d PA", minCost, maxCost);
            printAction(actY, col, "[H]", "Habilidad", artCost, minCost);
        }
        printAction(actY,     col2, "[D]",     "Defender",       "1 PA",  1);
        actY++;
        printAction(actY,     col,  "[SPACE]", "Terminar turno", "0 PA",  0);
        printAction(actY,     col2, "[R]",     "Huir",           "3 PA",  3);
        actY++;
        attron(COLOR_PAIR(1) | A_DIM);
        mvprintw(actY, col,  "[TAB] Cambiar objetivo");
        attroff(COLOR_PAIR(1) | A_DIM);

        // Enemies turn hint
        if (combat.getPhase() == CombatPhase::EnemyTurn) {
            attron(COLOR_PAIR(3) | A_BOLD);
            drawCentered(actY - 1, "~~ Turno del enemigo ~~");
            attroff(COLOR_PAIR(3) | A_BOLD);
        }

        // Game over hint
        if (combat.isOver()) {
            attron(COLOR_PAIR(2) | A_BOLD);
            drawCentered(actY, combat.playerWon()
                ? "VICTORIA!  Cualquier tecla para continuar"
                : "DERROTA!   Cualquier tecla para continuar");
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
    } else {
        // Arts submenu
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(actY++, 2, "=== Habilidades ===  (ESC para volver)");
        attroff(COLOR_PAIR(3) | A_BOLD);

        auto arts = player.getAvailableArts();
        for (int i = 0; i < static_cast<int>(arts.size()); i++) {
            const auto& art = arts[i];
            bool canAfford = (ap >= art.apCost) && (player.getMana() >= art.manaCost);
            bool selected  = (i == artSelection);

            if (selected && canAfford) attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            else if (selected)         attron(COLOR_PAIR(1) | A_BOLD | A_REVERSE);
            else if (!canAfford)       attron(COLOR_PAIR(1) | A_DIM);
            else                       attron(COLOR_PAIR(1));

            mvprintw(actY, 2, "[%d] %-18s  %d PA  %d MP  %s",
                     i + 1,
                     art.name.c_str(),
                     art.apCost,
                     art.manaCost,
                     art.description.c_str());

            attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE | COLOR_PAIR(1) | A_DIM);
            actY++;
        }

        attron(COLOR_PAIR(1) | A_DIM);
        mvprintw(actY, 2, "Flechas arriba/abajo para navegar  |  ENTER para confirmar");
        attroff(COLOR_PAIR(1) | A_DIM);
    }
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
