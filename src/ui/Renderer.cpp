#include "Renderer.hpp"
#include "ui/XpLoader.hpp"
#include <raylib.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <stdexcept>

// ─── colores TUI ─────────────────────────────────────────────────────────────
static constexpr Color COL_WHITE   = WHITE;
static constexpr Color COL_YELLOW  = { 255, 215,   0, 255 };
static constexpr Color COL_CYAN    = {   0, 200, 200, 255 };
static constexpr Color COL_GREEN   = {   0, 200,  80, 255 };
static constexpr Color COL_RED     = { 220,  50,  50, 255 };
static constexpr Color COL_GRAY    = { 100, 100, 100, 255 };
static constexpr Color COL_ORANGE  = { 255, 165,   0, 255 };
static constexpr Color COL_BLACK   = BLACK;

// ─── helpers ─────────────────────────────────────────────────────────────────

const char* Renderer::className(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior: return "Guerrero";
        case PlayerClass::Mage:    return "Mago";
        case PlayerClass::Ranger:  return "Ranger";
    }
    return "";
}

Color Renderer::colorFromPair(int pair) {
    switch (pair) {
        case 2: return COL_YELLOW;
        case 3: return COL_CYAN;
        case 4: return COL_GREEN;
        case 5: return COL_CYAN;
        case 6: return COL_RED;
        case 7: return COL_GRAY;
        default: return COL_WHITE;
    }
}

// ─── primitivas de layout ────────────────────────────────────────────────────

void Renderer::drawBorder(TerminalScreen& scr, int col, int row, int w, int h, Color c) {
    scr.put(col,         row,         0x250C, c);
    scr.put(col + w - 1, row,         0x2510, c);
    scr.put(col,         row + h - 1, 0x2514, c);
    scr.put(col + w - 1, row + h - 1, 0x2518, c);
    for (int x = 1; x < w - 1; x++) {
        scr.put(col + x, row,         0x2500, c);
        scr.put(col + x, row + h - 1, 0x2500, c);
    }
    for (int y = 1; y < h - 1; y++) {
        scr.put(col,         row + y, 0x2502, c);
        scr.put(col + w - 1, row + y, 0x2502, c);
    }
}

void Renderer::drawHSep(TerminalScreen& scr, int col, int row, int w, Color c) {
    for (int x = 0; x < w; x++)
        scr.put(col + x, row, 0x2500, c);
}

void Renderer::drawVSep(TerminalScreen& scr, int col, int row, int h, Color c) {
    for (int y = 0; y < h; y++)
        scr.put(col, row + y, 0x2502, c);
}

void Renderer::drawCentered(TerminalScreen& scr, int row, int col0, int w,
                             const std::string& str, Color fg, uint8_t flags) {
    int len = static_cast<int>(str.size());
    int x   = col0 + std::max(0, (w - len) / 2);
    scr.putStr(x, row, str, fg, COL_BLACK, flags);
}

void Renderer::drawStatBar(TerminalScreen& scr, int col, int row,
                            int value, int max, int barW, Color c) {
    int fill = (max > 0) ? (value * barW / max) : 0;
    fill = std::max(0, std::min(fill, barW));
    for (int i = 0; i < barW; i++)
        scr.put(col + i, row, i < fill ? 0x2588 : 0x2591, c);
}

// ─── drawMap ─────────────────────────────────────────────────────────────────

void Renderer::drawMap(TerminalScreen& scr, int col, int row,
                       int viewW, int viewH,
                       const Map& map, const std::vector<MapEntity>& entities) {
    Position pp = map.getPlayerPos();
    int camX = pp.x - viewW / 2;
    int camY = pp.y - viewH / 2;

    for (int sy = 0; sy < viewH; sy++) {
        for (int sx = 0; sx < viewW; sx++) {
            int mx = camX + sx, my = camY + sy;
            int dc = col + sx, dr = row + sy;
            if (mx < 0 || mx >= map.width() || my < 0 || my >= map.height()) continue;
            const Tile& tile = map.at(mx, my);
            if (!tile.explored) continue;

            if (mx == pp.x && my == pp.y) {
                scr.put(dc, dr, '@', COL_YELLOW, COL_BLACK, CELL_BOLD);
                continue;
            }
            if (tile.visible) {
                bool drew = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        scr.put(dc, dr, ent.glyph, colorFromPair(ent.colorPair),
                                COL_BLACK, ent.bold ? CELL_BOLD : 0);
                        drew = true;
                        break;
                    }
                }
                if (drew) continue;
            }
            if (tile.type == TileType::SecretWall) {
                scr.put(dc, dr, '#', COL_GRAY, COL_BLACK, CELL_DIM);
                continue;
            }
            Color tc = tile.visible ? COL_WHITE : COL_GRAY;
            uint8_t tf = tile.visible ? 0 : CELL_DIM;
            scr.put(dc, dr, tile.glyph, tc, COL_BLACK, tf);
        }
    }
}

// ─── drawPortrait ─────────────────────────────────────────────────────────────

void Renderer::drawPortrait(TerminalScreen& scr, int col, int row,
                             const char* filename) {
    if (!filename) {
        scr.putStr(col + 1, row + 1, "[sin retrato]", COL_GRAY, COL_BLACK, CELL_DIM);
        return;
    }
    try {
        XpFile xp = loadXp(std::string(ASSETS_DIR) + "/art/" + filename);
        if (xp.layers.empty()) throw std::runtime_error("vacio");
        xpDrawHalfBlock(scr, xp.layers[0], col, row);
    } catch (...) {
        scr.putStr(col + 1, row + 1, "[retrato N/A]", COL_GRAY, COL_BLACK, CELL_DIM);
    }
}

// ─── HUD helpers ─────────────────────────────────────────────────────────────

void Renderer::drawHudPanel(TerminalScreen& scr, int col, int row,
                             const Player& player) {
    int r = row;
    auto put = [&](const std::string& s, Color c, uint8_t f = 0) {
        scr.putStr(col + 1, r++, s, c, COL_BLACK, f);
    };
    put(player.getName().substr(0, 14), COL_CYAN, CELL_BOLD);
    put(std::string("[") + className(player) + "]", COL_WHITE);
    put("Nivel " + std::to_string(player.getLevel()), COL_WHITE);
    drawHSep(scr, col, r++, 20);
    put("HP " + std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp()), COL_GREEN);
    drawStatBar(scr, col + 1, r++, player.getHp(), player.getMaxHp(), 14, COL_GREEN);
    put("MP " + std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana()), COL_CYAN);
    drawStatBar(scr, col + 1, r++, player.getMana(), player.getMaxMana(), 14, COL_CYAN);
    put("XP " + std::to_string(player.getXp()), COL_YELLOW);
    drawHSep(scr, col, r++, 20);
    put("ATK " + std::to_string(player.getAttack()), COL_WHITE);
    put("DEF " + std::to_string(player.getDefense()), COL_WHITE);
    drawHSep(scr, col, r++, 20);
    put("EQUIPO", COL_WHITE, CELL_BOLD);
    put("Arma    : " + (player.getEquippedWeapon()
        ? player.getEquippedWeapon()->name.substr(0, 10) : std::string("-")),
        player.getEquippedWeapon() ? COL_YELLOW : COL_GRAY,
        player.getEquippedWeapon() ? 0 : CELL_DIM);
    put("Armadura: " + (player.getEquippedArmor()
        ? player.getEquippedArmor()->name.substr(0, 10) : std::string("-")),
        player.getEquippedArmor() ? COL_GREEN : COL_GRAY,
        player.getEquippedArmor() ? 0 : CELL_DIM);
    drawHSep(scr, col, r++, 20);
    put("$ " + std::to_string(player.getCoins()) + " monedas", COL_YELLOW, CELL_BOLD);
    put("k " + std::to_string(player.getKeys()) + " llave(s)", COL_CYAN);
    put("+ " + std::to_string(player.countConsumables()) + " pocion(es)", COL_GREEN);
    put("Piso " + std::to_string(player.getDungeonFloor()), COL_GRAY, CELL_DIM);
    drawHSep(scr, col, r++, 20);
    put("WASD  mover",   COL_GRAY, CELL_DIM);
    put("  P   pocion",  COL_GRAY, CELL_DIM);
    put("  I   mochila", COL_GRAY, CELL_DIM);
    put("  Q   salir",   COL_GRAY, CELL_DIM);
}

void Renderer::drawHudBar(TerminalScreen& scr, int row, const Player& player) {
    drawHSep(scr, 0, row, scr.cols());
    int c = 1;
    auto a = [&](const std::string& s, Color col, uint8_t f = 0) {
        c += scr.putStr(c, row + 1, s, col, COL_BLACK, f);
    };
    a(player.getName().substr(0, 12), COL_CYAN, CELL_BOLD);
    a(" [" + std::string(className(player)) + "] Nv." + std::to_string(player.getLevel()), COL_WHITE);
    a("  HP:", COL_GREEN, CELL_BOLD);
    a(std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp()), COL_GREEN);
    a("  MP:", COL_CYAN, CELL_BOLD);
    a(std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana()), COL_CYAN);

    c = 1;
    auto b = [&](const std::string& s, Color col, uint8_t f = 0) {
        c += scr.putStr(c, row + 2, s, col, COL_BLACK, f);
    };
    b("ATK:" + std::to_string(player.getAttack()), COL_GRAY, CELL_DIM);
    b(" DEF:" + std::to_string(player.getDefense()), COL_GRAY, CELL_DIM);
    b("  $" + std::to_string(player.getCoins()), COL_YELLOW);
    b("  k" + std::to_string(player.getKeys()), COL_CYAN);
    b("  +" + std::to_string(player.countConsumables()), COL_GREEN);
    b("  WASD:mover  P:pocion  I:mochila  Q:salir", COL_GRAY, CELL_DIM);
}

// ─── drawTitle ───────────────────────────────────────────────────────────────

void Renderer::drawTitle(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    std::ifstream f(ASSETS_DIR "/title.txt");
    std::string line;
    int ty = cy - 8;
    while (std::getline(f, line)) {
        int x = cx - static_cast<int>(line.size()) / 2;
        scr.putStr(x, ty++, line, COL_CYAN, COL_BLACK, CELL_BOLD);
    }
    ty++;
    scr.putStr(cx - 16, ty++, "~ Un RPG de mazmorra y sombras ~", COL_WHITE);
    ty++;
    const char* opts[] = { "Nueva Partida", "Salir" };
    for (int i = 0; i < 2; i++) {
        std::string label = std::string("  ") + opts[i] + "  ";
        int x = cx - static_cast<int>(label.size()) / 2;
        Color c = (i == selection) ? COL_YELLOW : COL_WHITE;
        uint8_t fl = (i == selection) ? (CELL_BOLD | CELL_INVERTED) : 0;
        scr.putStr(x, ty++, label, c, COL_BLACK, fl);
    }
    ty++;
    drawCentered(scr, ty, 0, scr.cols(),
                 "Flechas para navegar  |  ENTER para confirmar",
                 COL_GRAY, CELL_DIM);
}

// ─── drawNameInput ────────────────────────────────────────────────────────────

void Renderer::drawNameInput(TerminalScreen& scr, const std::string& name) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    drawCentered(scr, cy - 4, 0, scr.cols(), "T E N E B R A R I U M",
                 COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 2, 0, scr.cols(), "Ingresa el nombre de tu personaje:", COL_WHITE);
    std::string display = "> " + name + "_";
    drawCentered(scr, cy, 0, scr.cols(), display, COL_YELLOW);
    drawCentered(scr, cy + 2, 0, scr.cols(),
                 "ENTER para continuar  |  ESC para volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawClassSelect ─────────────────────────────────────────────────────────

struct ClassInfo {
    const char* name;
    const char* role;
    const char* desc;
    const char* portrait;
    int hp, atk, def, mana;
};
static constexpr ClassInfo kClasses[3] = {
    {"GUERRERO","Vanguardia","Maestro del combate cuerpo a cuerpo.",
     "warrior.xp", 120,15,8,20},
    {"MAGO","Arcano","Domina las artes magicas. Poder devastador.",
     "mago.xp", 70,8,3,100},
    {"RANGER","Explorador","Agil y versatil. Experto en trampas y arco.",
     nullptr, 90,12,5,50},
};

void Renderer::drawClassSelect(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2;
    drawCentered(scr, 1, 0, scr.cols(), "T E N E B R A R I U M", COL_CYAN, CELL_BOLD);
    drawCentered(scr, 2, 0, scr.cols(), "Elige tu clase:", COL_WHITE);

    int listCol = 2, boxW = 42, boxH = 8;
    for (int i = 0; i < 3; i++) {
        const auto& c = kClasses[i];
        bool sel = (i == selection);
        Color tc = sel ? COL_YELLOW : COL_GRAY;
        uint8_t tf = sel ? CELL_BOLD : CELL_DIM;
        int br = 4 + i * (boxH + 1);
        drawBorder(scr, listCol, br, boxW, boxH, tc);
        scr.putStr(listCol + 2, br + 1, c.name, tc, COL_BLACK, tf);
        scr.putStr(listCol + 2, br + 2,
                   std::string("[ ") + c.role + " ]", tc, COL_BLACK, tf);
        drawHSep(scr, listCol + 1, br + 3, boxW - 2, tc);
        scr.putStr(listCol + 2, br + 4,
            "HP:" + std::to_string(c.hp) +
            " ATK:" + std::to_string(c.atk) +
            " DEF:" + std::to_string(c.def) +
            " MP:" + std::to_string(c.mana), COL_GREEN, COL_BLACK, tf);
        scr.putStr(listCol + 2, br + 5, c.desc, tc, COL_BLACK, CELL_DIM);
    }

    int portCol = listCol + boxW + 2;
    drawPortrait(scr, portCol, 4, kClasses[selection].portrait);

    drawCentered(scr, scr.rows() - 2, 0, scr.cols(),
                 "Arriba/Abajo navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawHudSelect ────────────────────────────────────────────────────────────

void Renderer::drawHudSelect(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    drawCentered(scr, cy - 6, 0, scr.cols(), "T E N E B R A R I U M",
                 COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 4, 0, scr.cols(), "Elige el estilo de interfaz:", COL_WHITE);
    const char* names[] = { "Panel lateral", "Barra inferior" };
    for (int i = 0; i < 2; i++) {
        bool sel = (i == selection);
        Color c = sel ? COL_YELLOW : COL_GRAY;
        uint8_t f = sel ? (CELL_BOLD | CELL_INVERTED) : CELL_DIM;
        int x = (i == 0) ? cx - 20 : cx + 4;
        scr.putStr(x, cy, names[i], c, COL_BLACK, f);
    }
    drawCentered(scr, cy + 4, 0, scr.cols(),
                 "Izq/Der navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawExploration ─────────────────────────────────────────────────────────

void Renderer::drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message) {
    if (layout == HudLayout::Sidebar) {
        int panelW = 22;
        int mapW   = scr.cols() - panelW - 1;
        drawMap(scr, 0, 0, mapW, scr.rows(), map, entities);
        drawVSep(scr, mapW, 0, scr.rows());
        drawHudPanel(scr, mapW + 1, 0, player);
        if (!message.empty())
            drawCentered(scr, scr.rows() / 2, 0, mapW,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    } else {
        int hudH = 3;
        int mapH = scr.rows() - hudH;
        drawMap(scr, 0, 0, scr.cols(), mapH, map, entities);
        drawHudBar(scr, mapH, player);
        if (!message.empty())
            drawCentered(scr, mapH / 2, 0, scr.cols(),
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    }
}

// ─── drawCombat ──────────────────────────────────────────────────────────────

void Renderer::drawCombat(TerminalScreen& scr, const CombatSystem& combat,
                           const Player& player, bool showingArts, int artSelection) {
    int cols = scr.cols(), rows = scr.rows();
    int halfW = cols / 2;

    // Enemigos (izquierda)
    int er = 1;
    scr.putStr(1, er++, "ENEMIGOS", COL_WHITE, COL_BLACK, CELL_BOLD);
    drawHSep(scr, 0, er++, halfW);
    const auto& enemies = combat.getEnemies();
    for (int i = 0; i < static_cast<int>(enemies.size()); i++) {
        const auto& e = enemies[i];
        bool tgt = (i == combat.getCurrentTarget());
        if (!e->isAlive()) {
            scr.putStr(2, er++, e->getName() + " [MUERTO]", COL_GRAY, COL_BLACK, CELL_DIM);
            continue;
        }
        Color nc = tgt ? COL_YELLOW : COL_WHITE;
        scr.putStr(2, er++, (tgt ? "> " : "  ") + e->getName(),
                   nc, COL_BLACK, tgt ? CELL_BOLD : 0);
        scr.putStr(2, er, "HP " + std::to_string(e->getHp()) + "/" +
                   std::to_string(e->getMaxHp()) + " ", COL_GREEN);
        drawStatBar(scr, 14, er++, e->getHp(), e->getMaxHp(), 12, COL_GREEN);
        const auto& efx = combat.getEnemyEffects(i);
        std::string tags;
        for (const auto& fx : efx) {
            if (fx.type == StatusEffect::Type::Poisoned)    tags += "[VEN]";
            if (fx.type == StatusEffect::Type::Frozen)      tags += "[HIE]";
            if (fx.type == StatusEffect::Type::TrapPending) tags += "[TRP]";
        }
        if (!tags.empty()) scr.putStr(2, er++, tags, COL_YELLOW, COL_BLACK, CELL_DIM);
        er++;
    }

    // Log (derecha)
    drawVSep(scr, halfW, 0, rows - 8);
    int lr = 1;
    scr.putStr(halfW + 2, lr++, "LOG", COL_WHITE, COL_BLACK, CELL_BOLD);
    drawHSep(scr, halfW + 1, lr++, halfW - 1);
    const auto& log = combat.getLog();
    int logStart = std::max(0, static_cast<int>(log.size()) - (rows - 12));
    for (int i = logStart; i < static_cast<int>(log.size()); i++)
        scr.putStr(halfW + 2, lr++, log[i], COL_WHITE);

    // Barra jugador
    int br = rows - 7;
    drawHSep(scr, 0, br++, cols);
    int ap = combat.getCurrentAp(), maxAp = combat.getMaxAp();
    std::string apStr = "PA: ";
    for (int i = 0; i < maxAp; i++) apStr += (i < ap) ? "\xe2\x97\x8f " : "\xe2\x97\x8b ";
    apStr += "(" + std::to_string(ap) + "/" + std::to_string(maxAp) + ")";

    const auto& pfx = combat.getPlayerEffects();
    std::string ptags;
    for (const auto& fx : pfx) {
        if (fx.type == StatusEffect::Type::Defending)      ptags += "[DEF]";
        if (fx.type == StatusEffect::Type::DefendingHeavy) ptags += "[ESC]";
        if (fx.type == StatusEffect::Type::AttackBoosted)  ptags += "[ATK+]";
    }

    scr.putStr(1, br, player.getName() + " [" + className(player) + "]",
               COL_CYAN, COL_BLACK, CELL_BOLD);
    scr.putStr(20, br, "HP:", COL_GREEN, COL_BLACK, CELL_BOLD);
    scr.putStr(24, br, std::to_string(player.getHp()) + "/" +
               std::to_string(player.getMaxHp()), COL_GREEN);
    scr.putStr(35, br, "MP:", COL_CYAN, COL_BLACK, CELL_BOLD);
    scr.putStr(39, br++, std::to_string(player.getMana()) + "/" +
               std::to_string(player.getMaxMana()), COL_CYAN);
    scr.putStr(1, br++, apStr, COL_YELLOW);
    if (!ptags.empty()) scr.putStr(1, br++, ptags, COL_YELLOW, COL_BLACK, CELL_DIM);
    drawHSep(scr, 0, br++, cols);

    if (!showingArts) {
        scr.putStr(1,        br,   "[A] Atacar  1PA", ap >= 1 ? COL_WHITE : COL_GRAY);
        scr.putStr(cols / 2, br++, "[H] Habilidad", COL_WHITE);
        scr.putStr(1,        br,   "[F] At.Fuerte 2PA", ap >= 2 ? COL_WHITE : COL_GRAY);
        scr.putStr(cols / 2, br++, "[D] Defender  1PA", ap >= 1 ? COL_WHITE : COL_GRAY);
        scr.putStr(1,        br,   "[SPACE] Fin turno", COL_WHITE);
        scr.putStr(cols / 2, br++, "[R] Huir  3PA", ap >= 3 ? COL_WHITE : COL_GRAY);
        scr.putStr(1, br, "[TAB] Cambiar objetivo", COL_GRAY, COL_BLACK, CELL_DIM);
        if (combat.getPhase() == CombatPhase::EnemyTurn)
            drawCentered(scr, br + 1, 0, cols, "~~ Turno del enemigo ~~", COL_CYAN, CELL_BOLD);
        if (combat.isOver()) {
            const char* msg = combat.playerWon()
                ? "VICTORIA!  Cualquier tecla para continuar"
                : "DERROTA!   Cualquier tecla para continuar";
            drawCentered(scr, br + 1, 0, cols, msg, COL_YELLOW, CELL_BOLD);
        }
    } else {
        scr.putStr(1, br++, "=== Habilidades ===  (ESC para volver)",
                   COL_CYAN, COL_BLACK, CELL_BOLD);
        auto arts = player.getAvailableArts();
        for (int i = 0; i < static_cast<int>(arts.size()); i++) {
            const auto& art = arts[i];
            bool canAfford = (ap >= art.apCost) && (player.getMana() >= art.manaCost);
            bool sel = (i == artSelection);
            std::string line = "[" + std::to_string(i + 1) + "] " + art.name +
                "  " + std::to_string(art.apCost) + "PA  " +
                std::to_string(art.manaCost) + "MP  " + art.description;
            Color rc = canAfford ? COL_WHITE : COL_GRAY;
            uint8_t rf = sel ? CELL_INVERTED : (canAfford ? 0 : CELL_DIM);
            if (sel && canAfford) rc = COL_YELLOW;
            scr.putStr(1, br++, line, rc, COL_BLACK, rf);
        }
        scr.putStr(1, br, "Arriba/Abajo  |  ENTER confirmar",
                   COL_GRAY, COL_BLACK, CELL_DIM);
    }
}

// ─── drawInventory ────────────────────────────────────────────────────────────

void Renderer::drawInventory(TerminalScreen& scr, const Player& player, int selection) {
    int cx = scr.cols() / 2;
    int w  = 60, sc2 = cx - w / 2, r = 2;
    drawBorder(scr, sc2, 1, w, scr.rows() - 2);
    scr.putStr(sc2 + 2, r++, "=== EQUIPO ===", COL_CYAN, COL_BLACK, CELL_BOLD);

    auto makeSlot = [&](int row, int sel, const char* label,
                        const std::optional<Item>& slot) {
        std::string line;
        Color c = COL_GRAY;
        uint8_t f = CELL_DIM;
        if (slot) {
            std::string tag = (slot->type == ItemType::Weapon) ? "[ARMA]   " : "[ARMADURA]";
            line = tag + " " + slot->name + "  +" + std::to_string(slot->statBonus)
                 + (slot->type == ItemType::Weapon ? " ATK" : " DEF");
            c = (slot->type == ItemType::Weapon) ? COL_YELLOW : COL_GREEN;
            f = 0;
        } else {
            line = std::string(label) + "  (vacio)";
        }
        if (selection == sel) f |= CELL_INVERTED;
        scr.putStr(sc2 + 2, row, line, c, COL_BLACK, f);
    };
    makeSlot(r++, 0, "Arma    ", player.getEquippedWeapon());
    makeSlot(r++, 1, "Armadura", player.getEquippedArmor());
    drawHSep(scr, sc2 + 1, r++, w - 2);

    scr.putStr(sc2 + 2, r++, "=== MOCHILA ===", COL_CYAN, COL_BLACK, CELL_BOLD);
    const auto& items = player.getInventory().items();
    if (items.empty()) {
        scr.putStr(sc2 + 2, r++, "(vacia)", COL_GRAY, COL_BLACK, CELL_DIM);
    } else {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            const auto& item = items[i];
            std::string tag; Color ic = COL_WHITE;
            switch (item.type) {
                case ItemType::Weapon:     tag = "[ARMA]   "; ic = COL_YELLOW; break;
                case ItemType::Armor:      tag = "[ARMADURA]"; ic = COL_GREEN; break;
                case ItemType::Consumable: tag = "[POCION] "; ic = COL_CYAN;  break;
                default:                  tag = "[MISC]   "; break;
            }
            std::string line = tag + " " + item.name;
            if (item.type != ItemType::Consumable)
                line += "  +" + std::to_string(item.statBonus)
                      + (item.type == ItemType::Weapon ? " ATK" : " DEF");
            else
                line += "  +" + std::to_string(item.statBonus) + " HP";
            uint8_t f = (selection == 2 + i) ? CELL_INVERTED : 0;
            scr.putStr(sc2 + 2, r++, line, ic, COL_BLACK, f);
        }
    }
    drawHSep(scr, sc2 + 1, r++, w - 2);
    scr.putStr(sc2 + 2, r++,
        "ATK:" + std::to_string(player.getAttack()) +
        "  DEF:" + std::to_string(player.getDefense()) +
        "  $" + std::to_string(player.getCoins()) +
        "  Slots:" + std::to_string(player.getInventory().usedSlots()) +
        "/" + std::to_string(player.getInventory().totalSlots()), COL_GREEN);
    scr.putStr(sc2 + 2, r,
               "[Arriba/Abajo] Navegar  [E] Equipar  [U] Usar  [ESC] Cerrar",
               COL_GRAY, COL_BLACK, CELL_DIM);
}

// ─── drawQuestLog ─────────────────────────────────────────────────────────────

void Renderer::drawQuestLog(TerminalScreen& scr) {
    drawCentered(scr, scr.rows() / 2, 0, scr.cols(),
                 "[Misiones] - en construccion", COL_WHITE);
}

// ─── drawGameOver ─────────────────────────────────────────────────────────────

void Renderer::drawGameOver(TerminalScreen& scr) {
    int cy = scr.rows() / 2;
    drawCentered(scr, cy - 1, 0, scr.cols(), "G A M E   O V E R",
                 COL_YELLOW, CELL_BOLD);
    drawCentered(scr, cy + 1, 0, scr.cols(), "ENTER para volver al menu",
                 COL_GRAY, CELL_DIM);
}

// ─── drawShop ─────────────────────────────────────────────────────────────────

void Renderer::drawShop(TerminalScreen& scr, const std::vector<ShopItem>& stock,
                         int selection, const Player& player,
                         const std::string& message) {
    int cx = scr.cols() / 2;
    int w = 62, sc2 = cx - w / 2, r = 2;
    int boxH = static_cast<int>(stock.size()) + 8;
    drawBorder(scr, sc2, 1, w, boxH, COL_ORANGE);
    scr.putStr(sc2 + 2, r++, "=== TIENDA DEL PISO ===", COL_ORANGE, COL_BLACK, CELL_BOLD);
    scr.putStr(sc2 + 2, r++,
               "$ " + std::to_string(player.getCoins()) + " monedas disponibles",
               COL_ORANGE);
    drawHSep(scr, sc2 + 1, r++, w - 2, COL_ORANGE);
    for (int i = 0; i < static_cast<int>(stock.size()); i++) {
        const auto& s = stock[i];
        bool sel = (i == selection);
        std::string line = (s.sold ? "[VENDIDO] " : "          ") +
                           s.item.name + "  -  " + std::to_string(s.price) +
                           " $   " + s.item.description;
        Color lc = s.sold ? COL_GRAY : (sel ? COL_ORANGE : COL_WHITE);
        uint8_t lf = s.sold ? CELL_DIM : (sel ? CELL_INVERTED : 0);
        scr.putStr(sc2 + 2, r++, line, lc, COL_BLACK, lf);
    }
    drawHSep(scr, sc2 + 1, r++, w - 2, COL_ORANGE);
    if (!message.empty())
        scr.putStr(sc2 + 2, r++, message, COL_GREEN, COL_BLACK, CELL_BOLD);
    scr.putStr(sc2 + 2, r,
               "Arriba/Abajo navegar  |  ENTER comprar  |  ESC salir",
               COL_GRAY, COL_BLACK, CELL_DIM);
}
