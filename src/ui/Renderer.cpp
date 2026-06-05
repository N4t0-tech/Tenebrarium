#include "Renderer.hpp"
#include "ui/XpLoader.hpp"
#include "core/Assets.hpp"
#include <raylib.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

// ─── colores TUI ─────────────────────────────────────────────────────────────
static constexpr Color COL_WHITE   = WHITE;
static constexpr Color COL_YELLOW  = { 255, 230,  60, 255 };
static constexpr Color COL_CYAN    = {  60, 230, 240, 255 };
static constexpr Color COL_BLUE    = {  80, 150, 255, 255 };
static constexpr Color COL_GREEN   = {  60, 230, 110, 255 };
static constexpr Color COL_RED     = { 255,  80,  80, 255 };
static constexpr Color COL_GRAY    = { 200, 200, 200, 255 };
static constexpr Color COL_MAGENTA = { 220,  80, 255, 255 };
static constexpr Color COL_DK_GREEN= {  40, 160,  60, 255 };
static constexpr Color COL_DK_GRAY = { 120, 120, 120, 255 };
static constexpr Color COL_ORANGE  = { 255, 185,  40, 255 };

static constexpr Color COL_BLACK   = BLACK;

static constexpr Color FG_GREEN = { 22, 38, 24, 255 };
static constexpr Color WG_GREEN = { 40, 36, 18, 255 };
static constexpr Color DG_GREEN = { 14, 22, 16, 255 };
static constexpr Color FG_MONO  = { 28, 28, 32, 255 };
static constexpr Color WG_MONO  = { 42, 42, 46, 255 };
static constexpr Color DG_MONO  = { 16, 16, 20, 255 };

static constexpr Color TILE_GREEN = { 180, 210, 170, 255 };
static constexpr Color DIM_GREEN  = {  55,  75,  55, 255 };
static constexpr Color TILE_MONO  = { 220, 220, 220, 255 };
static constexpr Color DIM_MONO   = {  80,  80,  80, 255 };

static constexpr Color COL_COLD_BG = { 8, 10, 16, 255 };  // negro frío sutil

// ─── helpers ─────────────────────────────────────────────────────────────────

const char* Renderer::className(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior: return "Guerrero";
        case PlayerClass::Mage:    return "Mago";
        case PlayerClass::Ranger:  return "Ranger";
        case PlayerClass::Halley:  return "Halley";
        case PlayerClass::Nato:    return "Nato";
    }
    return "";
}

Color Renderer::colorFromPair(int pair) {
    switch (pair) {
        case  2: return COL_YELLOW;
        case  3: return COL_CYAN;
        case  4: return COL_GREEN;
        case  5: return COL_WHITE;
        case  6: return COL_RED;
        case  7: return COL_GRAY;
        case  8: return COL_MAGENTA;
        case  9: return COL_ORANGE;
        case 10: return COL_DK_GREEN;
        case 11: return COL_DK_GRAY;
        default: return COL_WHITE;
    }
}

// ─── primitivas de layout ────────────────────────────────────────────────────

void Renderer::drawBorder(TerminalScreen& scr, int col, int row, int w, int h, Color c) {
    scr.put(col,         row,         0x2554, c);
    scr.put(col + w - 1, row,         0x2557, c);
    scr.put(col,         row + h - 1, 0x255A, c);
    scr.put(col + w - 1, row + h - 1, 0x255D, c);
    for (int x = 1; x < w - 1; x++) {
        scr.put(col + x, row,         0x2550, c);
        scr.put(col + x, row + h - 1, 0x2550, c);
    }
    for (int y = 1; y < h - 1; y++) {
        scr.put(col,         row + y, 0x2551, c);
        scr.put(col + w - 1, row + y, 0x2551, c);
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

Color Renderer::colorForPlayerClass(PlayerClass pc) {
    switch (pc) {
        case PlayerClass::Warrior: return COL_YELLOW;
        case PlayerClass::Mage:    return COL_CYAN;
        case PlayerClass::Ranger:  return COL_GREEN;
        case PlayerClass::Halley:  return COL_MAGENTA;
        case PlayerClass::Nato:    return COL_RED;
    }
    return COL_YELLOW;
}

static const char* portraitForClass(PlayerClass pc) {
    switch (pc) {
        case PlayerClass::Warrior: return "mini-warrior.xp";
        case PlayerClass::Mage:    return "mini-wizard.xp";
        case PlayerClass::Ranger:  return "mini-ranger.xp";
        case PlayerClass::Halley:  return "mini-halley.xp";
        case PlayerClass::Nato:    return "mini-nato.xp";
    }
    return "mini-warrior.xp";
}

// ─── drawMap ─────────────────────────────────────────────────────────────────

void Renderer::drawMap(TerminalScreen& scr, int col, int row,
                       int viewW, int viewH,
                       const Map& map, const std::vector<MapEntity>& entities,
                       Color playerColor, int floor) {
    Position pp = map.getPlayerPos();
    int camX = pp.x - viewW / 2;
    int camY = pp.y - viewH / 2;

    auto applyFactor = [](Color c, float f) -> Color {
        return { (uint8_t)(c.r * f), (uint8_t)(c.g * f),
                 (uint8_t)(c.b * f), c.a };
    };

    float t = std::min(1.0f, (floor - 1) / 19.0f);
    auto lerp = [](Color a, Color b, float t) {
        return Color{ uint8_t(a.r + (b.r - a.r) * t),
                      uint8_t(a.g + (b.g - a.g) * t),
                      uint8_t(a.b + (b.b - a.b) * t), 255 };
    };
    Color colFloor   = lerp(FG_GREEN, FG_MONO, t);
    Color colWall    = lerp(WG_GREEN, WG_MONO, t);
    Color colDim     = lerp(DG_GREEN, DG_MONO, t);
    Color colTile    = lerp(TILE_GREEN, TILE_MONO, t);
    Color colDimGlyph = lerp(DIM_GREEN, DIM_MONO, t);

    static constexpr float FOV_RADIUS = 8.0f;

    // Rellenar todo el viewport con negro frío
    for (int sy = 0; sy < viewH; sy++)
        for (int sx = 0; sx < viewW; sx++)
            scr.put(col + sx, row + sy, ' ', COL_BLACK, COL_COLD_BG, 0);

    for (int sy = 0; sy < viewH; sy++) {
        for (int sx = 0; sx < viewW; sx++) {
            int mx = camX + sx, my = camY + sy;
            int dc = col + sx, dr = row + sy;
            if (mx < 0 || mx >= map.width() || my < 0 || my >= map.height()) continue;
            const Tile& tile = map.at(mx, my);
            if (!tile.explored) continue;

            if (mx == pp.x && my == pp.y) {
                scr.put(dc, dr, '@', playerColor, colFloor, CELL_BOLD);
                continue;
            }

            if (tile.visible) {
                float dx = (float)(mx - pp.x), dy = (float)(my - pp.y);
                float dist  = std::sqrt(dx*dx + dy*dy);
                float factor = std::max(0.45f, 1.0f - (dist / FOV_RADIUS) * 0.55f);

                bool drew = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        Color ec = applyFactor(colorFromPair(ent.colorPair), factor);
                        scr.put(dc, dr, ent.glyph, ec, colFloor,
                                ent.bold ? CELL_BOLD : 0);
                        drew = true;
                        break;
                    }
                }
                if (drew) continue;

                if (tile.type == TileType::SecretWall) {
                    scr.put(dc, dr, '#', applyFactor(COL_GRAY, factor), colWall, CELL_DIM);
                    continue;
                }
                Color tileBg = (tile.type == TileType::Floor || tile.type == TileType::Stairs) ? colFloor : colWall;
                scr.put(dc, dr, tile.glyph, applyFactor(colTile, factor), tileBg, 0);
            } else {
                // La pared secreta usa '#' (como pared normal) para no parpadear al salir del FOV
                char32_t g = (tile.type == TileType::SecretWall) ? '#' : tile.glyph;
                scr.put(dc, dr, g, colDimGlyph, colDim, 0);
            }
        }
    }
}

// ─── drawPortrait ─────────────────────────────────────────────────────────────

void Renderer::drawPortrait(TerminalScreen& scr, int col, int row,
                              const char* filename, float scale, Color tint, bool useGlyphs) {
    if (!filename) {
        scr.putStr(col + 1, row + 1, "[sin retrato]", COL_GRAY, COL_BLACK, CELL_DIM);
        return;
    }
    try {
        XpFile& xp = loadXpCached(assetsDir() + "art/" + filename);
        if (xp.layers.empty()) throw std::runtime_error("vacío");
        if (useGlyphs)
            xpDrawGlyphs(scr, xp.layers[0], col, row, scale);
        else
            xpDrawHalfBlock(scr, xp.layers[0], col, row, scale, tint);
    } catch (...) {
        scr.putStr(col + 1, row + 1, "[retrato N/A]", COL_GRAY, COL_BLACK, CELL_DIM);
    }
}

// ─── HUD helpers ─────────────────────────────────────────────────────────────

void Renderer::drawHudPanel(TerminalScreen& scr, int col, int row,
                             const Player& player, int mapZoom) {
    int r = row;
    auto put = [&](const std::string& s, Color c, uint8_t f = 0) {
        scr.putStr(col + 1, r++, s, c, COL_BLACK, f);
    };
    int sepW = 28;

    const char* pf = portraitForClass(player.getClass());
    if (pf) {
        static constexpr int kPortBorderW = 22;
        static constexpr int kPortBorderH = 12;
        int bx = col + (sepW - kPortBorderW) / 2;
        drawBorder(scr, bx, r, kPortBorderW, kPortBorderH);
        drawPortrait(scr, bx + 1, r + 1, pf, 1.0f);
    }
    r += 12;

    put(player.getName().substr(0, 14) + " [" + className(player)
        + "] Nv." + std::to_string(player.getLevel()), COL_CYAN, CELL_BOLD);
    drawHSep(scr, col, r++, sepW);
    put("HP " + std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp()), COL_GREEN);
    drawStatBar(scr, col + 1, r++, player.getHp(), player.getMaxHp(), 14, COL_GREEN);
    std::string mpLabel = player.getClass() == PlayerClass::Warrior ? "AG " : "MP ";
    put(mpLabel + std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana()), COL_CYAN);
    drawStatBar(scr, col + 1, r++, player.getMana(), player.getMaxMana(), 14, COL_CYAN);
    put("XP " + std::to_string(player.getXp()), COL_YELLOW);
    drawHSep(scr, col, r++, sepW);
    put("ATK " + std::to_string(player.getAttack()), COL_WHITE);
    put("DEF " + std::to_string(player.getDefense()), COL_WHITE);
    drawHSep(scr, col, r++, sepW);
    put("EQUIPO", COL_WHITE, CELL_BOLD);
    put("Arma    : " + (player.getEquippedWeapon()
        ? player.getEquippedWeapon()->name.substr(0, 10) : std::string("-")),
        player.getEquippedWeapon() ? COL_YELLOW : COL_GRAY,
        player.getEquippedWeapon() ? 0 : CELL_DIM);
    put("Armadura: " + (player.getEquippedArmor()
        ? player.getEquippedArmor()->name.substr(0, 10) : std::string("-")),
        player.getEquippedArmor() ? COL_GREEN : COL_GRAY,
        player.getEquippedArmor() ? 0 : CELL_DIM);
    drawHSep(scr, col, r++, sepW);
     put("$ " + std::to_string(player.getCoins()) + " monedas", COL_YELLOW, CELL_BOLD);
     put("+ " + std::to_string(player.countHpPotions()) + " poción(es)", COL_GREEN);
     if (player.getClass() == PlayerClass::Warrior)
         put("C " + std::to_string(player.getInventory().getItemCount("Cerveza")) + " cerveza(s)", COL_CYAN);
     else
         put("M " + std::to_string(player.countManaPotions()) + " maná", COL_BLUE);
     put("B " + std::to_string(player.getInventory().getItemCount("Bomba")) + " bomba(s)", COL_ORANGE);
     int fl = player.getDungeonFloor();
    std::string diff = fl <= 2 ? "Fácil" : fl <= 4 ? "Normal" : fl <= 6 ? "Difícil" : "Peligroso";
    Color dc = fl <= 2 ? COL_GREEN : fl <= 4 ? COL_YELLOW : COL_RED;
    put("Piso " + std::to_string(fl) + " [" + diff + "]", dc, CELL_DIM);
    drawHSep(scr, col, r++, sepW);
     put("WASD  mover",    COL_GRAY, CELL_DIM);
     put("  E   bomba",    COL_GRAY, CELL_DIM);
      put("  P   poción",   COL_GRAY, CELL_DIM);
     put("  R   tomar",   COL_GRAY, CELL_DIM);
     put("  I   mochila",  COL_GRAY, CELL_DIM);
     put("  M   misiones", COL_GRAY, CELL_DIM);
     put("  Esc salir",   COL_GRAY, CELL_DIM);
     put("+/-   zoom " + std::to_string(mapZoom) + "x", COL_GRAY, CELL_DIM);
}

// HUD inferior: 6 filas (separador + retrato 20×10 half-block + contenido compacto + controles)
void Renderer::drawHudBar(TerminalScreen& scr, int row, const Player& player, int mapZoom) {
    int cols = scr.cols();

    constexpr int kPortBorderW = 22;
    constexpr int kPortBorderH = 12;

    // Left: portrait box
    const char* pf = portraitForClass(player.getClass());
    if (pf) {
        drawBorder(scr, 0, row + 1, kPortBorderW, kPortBorderH);
        drawPortrait(scr, 1, row + 2, pf, 1.0f);
    }

    // Right: stats + controls box
    int rpX = kPortBorderW;
    int rpW = cols - rpX - 1;
    drawBorder(scr, rpX, row + 1, rpW, kPortBorderH);
    int rpC = rpX + 1;  // content column inside right panel

    // row 1: name + class + level
    scr.putStr(rpC, row + 1, player.getName().substr(0, 12) + " [" + className(player)
               + "] Nv." + std::to_string(player.getLevel()), COL_CYAN, COL_BLACK, CELL_BOLD);

    // row 2: HP + MP bars
    int c2 = rpC;
    auto a2 = [&](const std::string& s, Color col, uint8_t f = 0) {
        c2 += scr.putStr(c2, row + 2, s, col, COL_BLACK, f);
    };
    a2("HP:" + std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp()) + " ", COL_GREEN);
    float hpPct = (float)player.getHp() / player.getMaxHp();
    for (int i = 0; i < 8; i++)
        scr.put(c2++, row + 2, i < hpPct * 8 ? 0x2588 : 0x2591, COL_GREEN, COL_BLACK, 0);
    a2(" ", COL_GREEN);
    std::string mpLbl = player.getClass() == PlayerClass::Warrior ? "AG:" : "MP:";
    a2(" " + mpLbl + std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana()) + " ", COL_CYAN);
    float mpPct = (float)player.getMana() / player.getMaxMana();
    for (int i = 0; i < 8; i++)
        scr.put(c2++, row + 2, i < mpPct * 8 ? 0x2588 : 0x2591, COL_CYAN, COL_BLACK, 0);

    // row 3: stats + resources
    int c3 = rpC;
    auto a3 = [&](const std::string& s, Color col, uint8_t f = 0) {
        c3 += scr.putStr(c3, row + 3, s, col, COL_BLACK, f);
    };
    a3("Atk:" + std::to_string(player.getAttack()), COL_GRAY, CELL_DIM);
    a3(" Def:" + std::to_string(player.getDefense()), COL_GRAY, CELL_DIM);
    a3(" $" + std::to_string(player.getCoins()), COL_YELLOW);
    a3(" P:" + std::to_string(player.countHpPotions()), COL_GREEN);
    if (player.getClass() == PlayerClass::Warrior)
        a3(" C:" + std::to_string(player.getInventory().getItemCount("Cerveza")), COL_CYAN);
    else
        a3(" M:" + std::to_string(player.countManaPotions()), COL_BLUE);
    a3(" B:" + std::to_string(player.getInventory().getItemCount("Bomba")), COL_ORANGE);

    // row 4: floor + zoom
    int c4 = rpC;
    auto a4 = [&](const std::string& s, Color col, uint8_t f = 0) {
        c4 += scr.putStr(c4, row + 4, s, col, COL_BLACK, f);
    };
    int fl = player.getDungeonFloor();
    std::string diff = fl <= 2 ? "Fácil" : fl <= 4 ? "Normal" : fl <= 6 ? "Difícil" : "Peligroso";
    Color dc = fl <= 2 ? COL_GREEN : fl <= 4 ? COL_YELLOW : COL_RED;
    a4("Piso " + std::to_string(fl) + " [" + diff + "]", dc, CELL_DIM);
    a4("  [+/-]zoom:" + std::to_string(mapZoom), COL_GRAY, CELL_DIM);

    // row 5: separator
    drawHSep(scr, rpX + 1, row + 5, rpW - 2, COL_GRAY);

    // row 6-7: controls
    scr.putStr(rpC, row + 6, "[WASD]Mover  [E]Bomba  [R]Tomar  [P]Poc  [I]Inv",
               COL_GRAY, COL_BLACK, CELL_DIM);
    scr.putStr(rpC, row + 7, "[M]Mis  [Esc]Salir  [+/-]Zoom",
               COL_GRAY, COL_BLACK, CELL_DIM);
}

// ─── drawTitle ───────────────────────────────────────────────────────────────

void Renderer::drawTitle(TerminalScreen& scr, int selection, bool hasSave, bool blink) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    std::ifstream f(assetsDir() + "title.txt");
    std::string line;
    int ty = cy - 8;
    while (std::getline(f, line)) {
        int cols = 0;
        for (unsigned char c : line)
            if ((c & 0xC0) != 0x80) cols++;
        int x = cx - cols / 2;
        scr.putStr(x, ty++, line, COL_GRAY, COL_BLACK, CELL_BOLD);
    }
    ty++;
    scr.putStr(cx - 16, ty++, "~ Un RPG de mazmorra y sombras ~", COL_WHITE);
    ty++;
    const char* opts4[] = { "Continuar", "Nueva Partida", "Configuración", "Créditos", "Salir" };
    const char* opts3[] = { "Nueva Partida", "Configuración", "Créditos", "Salir" };
    int n = hasSave ? 5 : 4;
    const char** opts = hasSave ? opts4 : opts3;
    for (int i = 0; i < n; i++) {
        std::string label = std::string("  ") + opts[i] + "  ";
        int labelWidth = 0;
        for (unsigned char c : label)
            if ((c & 0xC0) != 0x80) labelWidth++;
        int x = cx - labelWidth / 2;
        bool sel = (i == selection);
        Color c = sel ? COL_YELLOW : COL_WHITE;
        uint8_t fl = sel ? CELL_BOLD : 0;
        // El item seleccionado parpadea entre invertido y normal
        if (sel && blink) fl |= CELL_INVERTED;
        scr.putStr(x, ty++, label, c, COL_BLACK, fl);
    }
    ty++;
    drawCentered(scr, ty, 0, scr.cols(),
                 "W/S navegar  |  ENTER para confirmar",
                 COL_GRAY, CELL_DIM);
}

// ─── drawCredits ─────────────────────────────────────────────────────────────

void Renderer::drawCredits(TerminalScreen& scr) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    std::ifstream f(assetsDir() + "title.txt");
    std::string line;
    int ty = cy - 8;
    while (std::getline(f, line)) {
        int cols = 0;
        for (unsigned char c : line)
            if ((c & 0xC0) != 0x80) cols++;
        int x = cx - cols / 2;
        scr.putStr(x, ty++, line, COL_GRAY, COL_BLACK, CELL_BOLD);
    }
    ty++;
    drawCentered(scr, ty++, 0, scr.cols(), "~ Créditos ~", COL_WHITE);

    int y = ty + 1;
    drawCentered(scr, y++, 0, scr.cols(), "Halley & Nato Co.", COL_WHITE);
    y++;
    drawCentered(scr, y,   0, scr.cols(), "ESC para volver", COL_GRAY, CELL_DIM);
}

// ─── drawNameInput ────────────────────────────────────────────────────────────

void Renderer::drawNameInput(TerminalScreen& scr, const std::string& name, bool blink) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    std::ifstream f(assetsDir() + "title.txt");
    std::string line;
    int ty = cy - 8;
    while (std::getline(f, line)) {
        int cols = 0;
        for (unsigned char c : line)
            if ((c & 0xC0) != 0x80) cols++;
        int x = cx - cols / 2;
        scr.putStr(x, ty++, line, COL_GRAY, COL_BLACK, CELL_BOLD);
    }
    ty++;
    scr.putStr(cx - 16, ty++, "~ Un RPG de mazmorra y sombras ~", COL_WHITE);
    ty++;
    drawCentered(scr, ty, 0, scr.cols(), "Ingresa el nombre de tu personaje:", COL_WHITE);
    ty++;
    std::string display = "> " + name + (blink ? "_" : " ");
    drawCentered(scr, ty, 0, scr.cols(), display, COL_YELLOW);
    ty += 2;
    drawCentered(scr, ty, 0, scr.cols(),
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
static constexpr ClassInfo kClasses[5] = {
    {"GUERRERO","Vanguardia","Maestro del combate cuerpo a cuerpo.",
     "warrior.xp", 120,20,8,100},
    {"MAGO","Arcano","Domina las artes mágicas. Poder devastador.",
     "mago.xp", 70,8,3,100},
    {"RANGER","Explorador","Ágil y versátil. Experto en trampas y arco.",
     "ranger.xp", 90,12,5,50},
    {"HALLEY","Equilibrio","La balanza que todo lo iguala.",
     "halley.xp", 130,18,7,170},
    {"NATO","Creador","Forjó el Tenebrarium con sus manos.",
     "nato.xp", 140,22,8,130},
};

void Renderer::drawClassSelect(TerminalScreen& scr, int selection, bool secretUnlocked) {
    drawCentered(scr, 1, 0, scr.cols(), "T E N E B R A R I U M", COL_CYAN, CELL_BOLD);
    drawCentered(scr, 2, 0, scr.cols(), "Elige tu clase:", COL_WHITE);

    if (secretUnlocked) {
        // ─── Modo clases secretas: retratos lado a lado arriba, ficha abajo ──
        static constexpr float kPortScale = 1.0f;
        static constexpr int kSrcSize = 60;
        int portW = kSrcSize;
        int portH = kSrcSize / 2;

        int gap = 4;
        int totalW = portW * 2 + gap;
        int startX = (scr.cols() - totalW) / 2;
        int portY = 4;

        auto drawSecretClass = [&](int classIdx, int x, const char* label) {
            bool sel = (selection == classIdx - 3);
            Color c = sel ? COL_YELLOW : COL_MAGENTA;
            drawBorder(scr, x - 1, portY - 1, portW + 2, portH + 2, c);
            Color tint = sel ? WHITE : Color{100, 100, 100, 255};
            drawPortrait(scr, x, portY, kClasses[classIdx].portrait, kPortScale, tint);
            drawCentered(scr, portY + portH, x - 1, portW + 2,
                         label, c, sel ? CELL_BOLD : 0);
        };
        drawSecretClass(3, startX,           "HALLEY");
        drawSecretClass(4, startX + portW + gap, "NATO");

        // Ficha de información debajo de los retratos
        int cardY = portY + portH + 3;
        const auto& c = kClasses[3 + selection];
        std::string stats = "HP:" + std::to_string(c.hp) +
                            " ATK:" + std::to_string(c.atk) +
                            " DEF:" + std::to_string(c.def) +
                            " MP:" + std::to_string(c.mana);
        std::string nameRole = std::string(c.name) + "  [ " + c.role + " ]";
        int cardW = std::max({ (int)nameRole.size(), (int)stats.size(),
                               (int)std::string(c.desc).size(),
                               20 }) + 4;
        int cardX = (scr.cols() - cardW) / 2;
        if (cardX < 1) cardX = 1;
        int cardH = 7;

        drawBorder(scr, cardX, cardY, cardW, cardH, COL_YELLOW);
        scr.putStr(cardX + 2, cardY + 1, nameRole, COL_YELLOW, COL_BLACK, CELL_BOLD);
        drawHSep(scr, cardX + 1, cardY + 2, cardW - 2, COL_YELLOW);
        scr.putStr(cardX + 2, cardY + 3, stats, COL_GREEN);
        scr.putStr(cardX + 2, cardY + 4, c.desc, COL_GRAY, COL_BLACK, CELL_DIM);
        scr.putStr(cardX + 2, cardY + 5, "~~ CLASE SECRETA ~~", COL_MAGENTA, COL_BLACK, CELL_DIM);

    } else {
        // ─── Modo clases normales: lista vertical + retrato a la derecha ────
        int nClasses = 3;
        int startIdx = 0;

        int maxContent = 0;
        for (int i = 0; i < nClasses; i++) {
            const auto& c = kClasses[startIdx + i];
            std::string stats = "HP:" + std::to_string(c.hp) +
                                " ATK:" + std::to_string(c.atk) +
                                " DEF:" + std::to_string(c.def) +
                                " MP:" + std::to_string(c.mana);
            std::string role  = std::string("[ ") + c.role + " ]";
            int w = std::max({ (int)std::string(c.name).size(),
                               (int)role.size(),
                               (int)stats.size(),
                               (int)std::string(c.desc).size() });
            maxContent = std::max(maxContent, w);
        }
        int boxW = maxContent + 4;
        int boxH = 7, listCol = 2;

        for (int i = 0; i < nClasses; i++) {
            const auto& c = kClasses[startIdx + i];
            bool sel = (i == selection);
            Color tc = sel ? COL_YELLOW : COL_GRAY;
            uint8_t tf = sel ? CELL_BOLD : CELL_DIM;
            int br = 4 + i * (boxH + 1);
            drawBorder(scr, listCol, br, boxW, boxH, tc);
            scr.putStr(listCol + 2, br + 1, c.name, tc, COL_BLACK, tf);
            scr.putStr(listCol + 2, br + 2,
                       std::string("[ ") + c.role + " ]", tc, COL_BLACK, tf);
            drawHSep(scr, listCol + 1, br + 3, boxW - 2, tc);
            std::string manaLabel = (i == 0) ? " AG:" : " MP:";
            scr.putStr(listCol + 2, br + 4,
                "HP:" + std::to_string(c.hp) +
                " ATK:" + std::to_string(c.atk) +
                " DEF:" + std::to_string(c.def) +
                manaLabel + std::to_string(c.mana), COL_GREEN, COL_BLACK, tf);
            scr.putStr(listCol + 2, br + 5, c.desc, tc, COL_BLACK, CELL_DIM);
        }

        static constexpr float kPortScale = 1.0f;
        static constexpr int kSrcSize = 60;
        int portW = kSrcSize;
        int portH = kSrcSize / 2;
        int spaceStart = listCol + boxW + 2;
        int portCol = spaceStart + (scr.cols() - spaceStart - portW) / 2;
        if (portCol < spaceStart) portCol = spaceStart;
        drawBorder(scr, portCol - 1, 3, portW + 2, portH + 2, COL_YELLOW);
        drawPortrait(scr, portCol, 4, kClasses[selection].portrait, kPortScale);
    }

    drawCentered(scr, scr.rows() - 2, 0, scr.cols(),
                 secretUnlocked
                     ? "A/D navegar  |  ENTER confirmar  |  ESC volver"
                     : "W/S navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawHudSelect ────────────────────────────────────────────────────────────

void Renderer::drawHudSelect(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    drawCentered(scr, cy - 10, 0, scr.cols(), "T E N E B R A R I U M",
                 COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 8, 0, scr.cols(), "Elige el estilo de interfaz:", COL_WHITE);

    // Preview sidebar (izquierda)
    {
        int px = cx - 38, py = cy - 6;
        int pw = 24, ph = 8;
        bool sel = (selection == 0);
        Color bc = sel ? COL_YELLOW : COL_GRAY;
        drawBorder(scr, px, py, pw, ph, bc);
        // mapa simulado
        const char* mapRows[] = {
            "· · · · · · · · ·",
            "· · · · · · · · ·",
            "· · · ·@· · · · ·",
            "· · · · · · · · ·",
            "· · · · · · · · ·",
        };
        for (int r = 0; r < 5; r++)
            scr.putStr(px + 1, py + 1 + r, mapRows[r], COL_GRAY, COL_BLACK, CELL_DIM);
        // separador vertical interior
        drawVSep(scr, px + pw - 7, py + 1, ph - 2, bc);
        // panel lateral simulado
        scr.putStr(px + pw - 5, py + 1, "Hero", COL_CYAN);
        scr.putStr(px + pw - 5, py + 2, "Lv.1", COL_WHITE);
        scr.putStr(px + pw - 5, py + 4, "HP\xe2\x96\x88\xe2\x96\x88", COL_GREEN);  // HP██
        scr.putStr(px + pw - 5, py + 5, "MP\xe2\x96\x91\xe2\x96\x91", COL_CYAN);  // MP░░
        // etiqueta
        std::string label = "Panel lateral";
        scr.putStr(px + (pw - (int)label.size()) / 2, py + ph,
                   label, bc, COL_BLACK, sel ? CELL_BOLD : CELL_DIM);
    }

    // Preview bottom bar (derecha)
    {
        int px = cx + 4, py = cy - 6;
        int pw = 26, ph = 8;
        bool sel = (selection == 1);
        Color bc = sel ? COL_YELLOW : COL_GRAY;
        drawBorder(scr, px, py, pw, ph, bc);
        // mapa simulado
        const char* mapRows[] = {
            "· · · · · · · · · · ·",
            "· · · · · · · · · · ·",
            "· · · · · @· · · · ·",
            "· · · · · · · · · · ·",
        };
        for (int r = 0; r < 4; r++)
            scr.putStr(px + 1, py + 1 + r, mapRows[r], COL_GRAY, COL_BLACK, CELL_DIM);
        // separador horizontal interior
        drawHSep(scr, px + 1, py + 5, pw - 2, bc);
        // barra inferior simulada
        scr.putStr(px + 1, py + 6, "Hero Lv1 HP\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91 MP\xe2\x96\x91\xe2\x96\x91",
                   COL_WHITE, COL_BLACK, CELL_DIM);
        // etiqueta
        std::string label = "Barra inferior";
        scr.putStr(px + (pw - (int)label.size()) / 2, py + ph,
                   label, bc, COL_BLACK, sel ? CELL_BOLD : CELL_DIM);
    }

    drawCentered(scr, cy + 4, 0, scr.cols(),
                 "A/D navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawSettings ──────────────────────────────────────────────────────────────

void Renderer::drawSettings(TerminalScreen& scr, int selection, HudLayout hud,
                            int mapZoom, bool shaderOn) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    drawCentered(scr, cy - 6, 0, scr.cols(), "T E N E B R A R I U M",
                 COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 4, 0, scr.cols(), "Configuración", COL_WHITE);

    struct Setting {
        const char* label;
        const char* value;
    };

    const char* hudVal = (hud == HudLayout::Sidebar) ? "Panel lateral" : "Barra inferior";
    const char* zoomVal = (mapZoom == 1) ? "1x" : (mapZoom == 2) ? "2x" : "3x";
    const char* shaderVal = shaderOn ? "On" : "Off";

    Setting items[] = {
        {"Estilo HUD",   hudVal},
        {"Zoom Mapa",    zoomVal},
        {"Shader CRT",   shaderVal},
        {"Volver",       ""},
    };

    for (int i = 0; i < 4; i++) {
        int y = cy - 1 + i * 2;
        bool sel = (i == selection);
        Color c = sel ? COL_YELLOW : COL_WHITE;
        uint8_t fl = sel ? CELL_BOLD : 0;
        std::string line = std::string("  ") + items[i].label;
        scr.putStr(cx - 16, y, line.c_str(), c, COL_BLACK, fl);
        if (items[i].value[0] != '\0') {
            std::string val = std::string("[") + items[i].value + "]";
            scr.putStr(cx + 4, y, val.c_str(), sel ? COL_CYAN : COL_GRAY, COL_BLACK, fl);
        }
    }

    drawCentered(scr, cy + 7, 0, scr.cols(),
                 "W/S navegar  |  ENTER cambiar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

void Renderer::drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message, int mapZoom) {
    if (layout == HudLayout::Sidebar) {
        int panelW = 30;
        int mapW   = scr.cols() - panelW - 1;
        drawMap(scr, 1, 1, mapW - 2, scr.rows() - 2, map, entities,
                colorForPlayerClass(player.getClass()), player.getDungeonFloor());
        drawBorder(scr, 0, 0, mapW, scr.rows());
        drawHudPanel(scr, mapW + 1, 0, player, mapZoom);
        if (!message.empty())
            drawCentered(scr, scr.rows() / 2, 1, mapW - 2,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    } else {
        int hudH = 13;
        int mapH = scr.rows() - hudH;
        drawMap(scr, 1, 1, scr.cols() - 2, mapH - 2, map, entities,
                colorForPlayerClass(player.getClass()), player.getDungeonFloor());
        drawBorder(scr, 0, 0, scr.cols(), mapH);
        drawHudBar(scr, mapH, player, mapZoom);
        if (!message.empty())
            drawCentered(scr, mapH / 2, 1, scr.cols() - 2,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    }
}

static Color hpColor(int hp, int maxHp) {
    float pct = (maxHp > 0) ? static_cast<float>(hp) / maxHp : 0.f;
    if (pct > 0.5f) return COL_GREEN;
    if (pct > 0.25f) return COL_YELLOW;
    return COL_RED;
}

// ─── log message coloring ────────────────────────────────────────────────────

static Color logColor(const std::string& msg) {
    // Victoria / derrota
    if (msg.find("Victoria") != std::string::npos ||
        msg.find("Victory") != std::string::npos ||
        msg.find("derrotado") != std::string::npos ||
        msg.find("derrotada") != std::string::npos)
        return COL_GREEN;
    // Daño crítico
    if (msg.find("CRITICO") != std::string::npos)
        return COL_ORANGE;
    // Daño recibido / veneno / trampa
    if (msg.find("daño") != std::string::npos ||
        msg.find("veneno") != std::string::npos ||
        msg.find("Trampa") != std::string::npos ||
        msg.find("envenena") != std::string::npos ||
        msg.find("drena") != std::string::npos)
        return COL_RED;
    // Curación / poción / regeneración
    if (msg.find("recupera") != std::string::npos ||
        msg.find("poción") != std::string::npos ||
        msg.find("cura") != std::string::npos ||
        (msg.find("HP") != std::string::npos && msg.find("recupera") != std::string::npos))
        return COL_GREEN;
    // Mana / habilidades
    if (msg.find("Mana") != std::string::npos ||
        msg.find("habilidad") != std::string::npos ||
        msg.find("usa") != std::string::npos ||
        msg.find("Habilidades") != std::string::npos)
        return COL_CYAN;
    // Defensa / escudo
    if (msg.find("defens") != std::string::npos ||
        msg.find("Escudo") != std::string::npos ||
        msg.find("Defendiendo") != std::string::npos)
        return COL_CYAN;
    // Fracasos / errores
    if (msg.find("insuficiente") != std::string::npos ||
        msg.find("falla") != std::string::npos ||
        msg.find("No hay") != std::string::npos)
        return COL_GRAY;
    // Fase de turno
    if (msg.find("===") != std::string::npos ||
        msg.find("PA disponibles") != std::string::npos)
        return COL_YELLOW;
    // Turno enemigo
    if (msg.find("Turno enemigo") != std::string::npos ||
        msg.find("enemigo ---") != std::string::npos)
        return COL_DK_GRAY;
    // Huye
    if (msg.find("huye") != std::string::npos ||
        msg.find("huir") != std::string::npos)
        return COL_GRAY;
    // Congelado / efectos de estado
    if (msg.find("congelado") != std::string::npos ||
        msg.find("ATK+") != std::string::npos ||
        msg.find("Boost") != std::string::npos)
        return COL_YELLOW;
    return COL_WHITE;
}

// ─── drawCombat ──────────────────────────────────────────────────────────────

void Renderer::drawCombat(TerminalScreen& scr, const CombatSystem& combat,
                           const Player& player, bool showingArts, int artSelection,
                           bool isBoss, int flashIdx) {
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
        if (isBoss && i == 0)
            scr.putStr(2, er++, "[JEFE]", COL_RED, COL_BLACK, CELL_BOLD);
        bool flash = (i == flashIdx);
        Color nc = (isBoss && i == 0) ? COL_RED : (tgt ? COL_YELLOW : COL_WHITE);
        Color nameBg = flash ? COL_RED : COL_BLACK;
        uint8_t nameFlags = (tgt ? CELL_BOLD : 0) | (flash ? CELL_INVERTED : 0);
        scr.putStr(2, er++, (tgt ? "> " : "  ") + e->getName(),
                   nc, nameBg, nameFlags);
        Color hpc = hpColor(e->getHp(), e->getMaxHp());
        scr.putStr(2, er, "HP " + std::to_string(e->getHp()) + "/" +
                   std::to_string(e->getMaxHp()) + " ", hpc);
        drawStatBar(scr, 14, er++, e->getHp(), e->getMaxHp(), 12, hpc);
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
        scr.putStr(halfW + 2, lr++, log[i], logColor(log[i]));

    // Bottom bar — calculate start row dynamically so content never overflows
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

    int bottomRows = 4;  // sep + name/HP/MP + AP/tags + sep
    if (showingArts) {
        auto artsList = player.getAvailableArts();
        bottomRows += 1 + static_cast<int>(artsList.size()) + 1;  // header + arts + hint
    } else {
        bottomRows += 3;  // action buttons
        if (combat.getPhase() == CombatPhase::EnemyTurn || combat.isOver())
            bottomRows += 1;
    }
    int br = std::max(0, rows - bottomRows);

    drawHSep(scr, 0, br++, cols);
    scr.putStr(1, br, player.getName() + " [" + className(player) + "]",
               COL_CYAN, COL_BLACK, CELL_BOLD);
    Color phpc = hpColor(player.getHp(), player.getMaxHp());
    scr.putStr(20, br, "HP:", phpc, COL_BLACK, CELL_BOLD);
    scr.putStr(24, br, std::to_string(player.getHp()) + "/" +
               std::to_string(player.getMaxHp()), phpc);
    std::string mpLbl2 = player.getClass() == PlayerClass::Warrior ? "AG:" : "MP:";
    scr.putStr(35, br, mpLbl2.c_str(), COL_CYAN, COL_BLACK, CELL_BOLD);
    scr.putStr(39, br++, std::to_string(player.getMana()) + "/" +
               std::to_string(player.getMaxMana()), COL_CYAN);
    if (!ptags.empty()) apStr += "  " + ptags;
    scr.putStr(1, br++, apStr, COL_YELLOW);
    drawHSep(scr, 0, br++, cols);

    if (!showingArts) {
        std::string mpCost = player.getClass() == PlayerClass::Warrior ? "AG" : "MP";
        scr.putStr(1,            br, "[1] Atacar      1PA", ap >= 1 ? COL_WHITE : COL_GRAY);
        scr.putStr(cols / 3,     br, "[2] At.Fuerte  2PA 8" + mpCost, ap >= 2 && player.getMana() >= 8 ? COL_WHITE : COL_GRAY);
        scr.putStr(2 * cols / 3, br++, "[3] Habilidades", COL_WHITE);
        scr.putStr(1,            br, "[4] Defender     1PA", ap >= 1 ? COL_WHITE : COL_GRAY);
        scr.putStr(cols / 3,     br, "[5] Poción (" + std::to_string(player.countHpPotions()) + ")  1PA", player.countHpPotions() > 0 && ap >= 1 ? COL_CYAN : COL_GRAY);
        scr.putStr(2 * cols / 3, br++, "[6] Huir        3PA", ap >= 3 ? COL_WHITE : COL_GRAY);
        scr.putStr(1,            br, "[7] Saquear     1PA", ap >= 1 ? COL_WHITE : COL_GRAY);
        scr.putStr(cols / 3,     br, "[SPACE] Fin turno", COL_WHITE);
        scr.putStr(2 * cols / 3, br++, "[TAB] Cambiar objetivo", COL_GRAY, COL_BLACK, CELL_DIM);
        if (combat.getPhase() == CombatPhase::EnemyTurn)
            drawCentered(scr, br, 0, cols, "~~ Turno del enemigo ~~", COL_CYAN, CELL_BOLD);
        if (combat.isOver()) {
            const char* msg = combat.playerWon()
                ? "VICTORIA!  Cualquier tecla para continuar"
                : "DERROTA!   Cualquier tecla para continuar";
            drawCentered(scr, br, 0, cols, msg, COL_YELLOW, CELL_BOLD);
        }
    } else {
        scr.putStr(1, br++, "=== Habilidades ===  (ESC para volver)",
                   COL_CYAN, COL_BLACK, CELL_BOLD);
        auto arts = player.getAvailableArts();
        for (int i = 0; i < static_cast<int>(arts.size()); i++) {
            const auto& art = arts[i];
            bool canAfford = (ap >= art.apCost) && (player.getMana() >= art.manaCost);
            bool sel = (i == artSelection);
            std::string mpUnit = player.getClass() == PlayerClass::Warrior ? "AG" : "MP";
            std::string line = "[" + std::to_string(i + 1) + "] " + art.name +
                "  " + std::to_string(art.apCost) + "PA  " +
                std::to_string(art.manaCost) + mpUnit + "  " + art.description;
            Color rc = canAfford ? COL_WHITE : COL_GRAY;
            uint8_t rf = sel ? CELL_INVERTED : (canAfford ? 0 : CELL_DIM);
            if (sel && canAfford) rc = COL_YELLOW;
            scr.putStr(1, br++, line, rc, COL_BLACK, rf);
        }
        scr.putStr(1, br, "W/S  |  ENTER confirmar",
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
            if (item.type == ItemType::Weapon) {
                tag = "[ARMA]   "; ic = COL_YELLOW;
            } else if (item.type == ItemType::Armor) {
                tag = "[ARMADURA]"; ic = COL_GREEN;
            } else if (item.type == ItemType::Consumable && item.statBonus > 0) {
                tag = "[POCIÓN] "; ic = COL_CYAN;
            } else if (item.type == ItemType::Consumable) {
                tag = "[MANA]   "; ic = COL_BLUE;
            } else if (item.type == ItemType::Bomb) {
                tag = "[BOMBA]  "; ic = COL_ORANGE;
            } else {
                tag = "[MISC]   ";
            }
            std::string line = tag + " " + item.name;
            if (item.quantity > 1)
                line += " x" + std::to_string(item.quantity);
            if (item.type != ItemType::Consumable && item.type != ItemType::Bomb)
                line += "  +" + std::to_string(item.statBonus)
                      + (item.type == ItemType::Weapon ? " ATK" : " DEF");
            else if (item.type == ItemType::Consumable && item.statBonus > 0)
                line += "  +" + std::to_string(item.statBonus) + " HP";
            else if (item.type == ItemType::Consumable)
                line += "  +50% " + std::string(player.getClass() == PlayerClass::Warrior ? "AG" : "MP");
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
               "[W/S] Navegar  [E] Equipar  [U] Usar  [ESC] Cerrar",
               COL_GRAY, COL_BLACK, CELL_DIM);
}

// ─── drawQuestLog ─────────────────────────────────────────────────────────────

void Renderer::drawQuestLog(TerminalScreen& scr,
                             const std::vector<Quest>& quests, int selection) {
    int w  = std::min(scr.cols() - 4, 70);
    int x0 = (scr.cols() - w) / 2;
    int y0 = 1;
    int h  = scr.rows() - 2;
    drawBorder(scr, x0, y0, w, h);
    drawCentered(scr, y0, x0, w, " DIARIO DE MISIONES ", COL_CYAN, CELL_BOLD);

    if (quests.empty()) {
        drawCentered(scr, y0 + h / 2, x0, w, "(Sin misiones activas)", COL_GRAY, CELL_DIM);
        drawCentered(scr, y0 + h - 1, x0, w, "ESC volver", COL_GRAY, CELL_DIM);
        return;
    }

    // Lista izquierda
    int listW  = w / 2 - 1;
    int detailX = x0 + listW + 1;
    int detailW = w - listW - 1;
    drawVSep(scr, x0 + listW, y0 + 1, h - 2);

    int listY = y0 + 1;
    for (int i = 0; i < static_cast<int>(quests.size()); i++) {
        const auto& q = quests[i];
        Color c; std::string icon;
        switch (q.status) {
            case QuestStatus::Completed:  c = COL_GREEN;  icon = "[+] "; break;
            case QuestStatus::InProgress: c = COL_YELLOW; icon = "[~] "; break;
            case QuestStatus::Failed:     c = COL_RED;    icon = "[x] "; break;
            default:                      c = COL_GRAY;   icon = "[ ] "; break;
        }
        std::string line = icon + q.title;
        if ((int)line.size() > listW - 2) line = line.substr(0, listW - 5) + "...";
        uint8_t f = (i == selection) ? CELL_INVERTED : 0;
        scr.putStr(x0 + 1, listY + i, line, c, COL_BLACK, f);
    }

    // Detalle derecho
    if (selection >= 0 && selection < (int)quests.size()) {
        const auto& q = quests[selection];
        int dy = y0 + 1;
        // Título
        std::string title = q.title;
        if ((int)title.size() > detailW - 2) title = title.substr(0, detailW - 5) + "...";
        scr.putStr(detailX + 1, dy++, title, COL_YELLOW, COL_BLACK, CELL_BOLD);
        dy++;
        // Estado
        std::string statusStr;
        Color sc;
        switch (q.status) {
            case QuestStatus::Completed:  statusStr = "COMPLETADA";  sc = COL_GREEN;  break;
            case QuestStatus::InProgress: statusStr = "EN PROGRESO"; sc = COL_YELLOW; break;
            case QuestStatus::Failed:     statusStr = "FALLIDA";     sc = COL_RED;    break;
            default:                      statusStr = "PENDIENTE";   sc = COL_GRAY;   break;
        }
        scr.putStr(detailX + 1, dy++, statusStr, sc, COL_BLACK, CELL_BOLD);
        dy++;
        // Descripción (wrap simple)
        std::string desc = q.description;
        while (!desc.empty() && dy < y0 + h - 4) {
            int take = std::min((int)desc.size(), detailW - 2);
            // no cortar a mitad de palabra
            if (take < (int)desc.size() && desc[take] != ' ')
                while (take > 0 && desc[take-1] != ' ') take--;
            scr.putStr(detailX + 1, dy++, desc.substr(0, take), COL_WHITE);
            desc = (take < (int)desc.size()) ? desc.substr(take) : "";
            if (!desc.empty() && desc[0] == ' ') desc = desc.substr(1);
        }
        dy++;
        // Objetivos
        scr.putStr(detailX + 1, dy++, "Objetivos:", COL_CYAN, COL_BLACK, CELL_BOLD);
        for (const auto& obj : q.objectives) {
            if (dy >= y0 + h - 2) break;
            std::string objLine = (obj.completed ? "\xe2\x96\xa0 " : "\xe2\x96\xa1 ") + obj.description;
            Color oc = obj.completed ? COL_GREEN : COL_GRAY;
            if ((int)objLine.size() > detailW - 2) objLine = objLine.substr(0, detailW - 5) + "...";
            scr.putStr(detailX + 1, dy++, objLine, oc);
        }
        // Recompensas
        if (dy < y0 + h - 2) {
            dy++;
            scr.putStr(detailX + 1, dy, "XP: " + std::to_string(q.xpReward) +
                       "   Oro: " + std::to_string(q.goldReward), COL_YELLOW);
        }
    }

    drawCentered(scr, y0 + h - 1, x0, w,
                 " W/S navegar  |  ESC volver ", COL_GRAY, CELL_DIM);
}

// ─── drawGameOver ─────────────────────────────────────────────────────────────

void Renderer::drawGameOver(TerminalScreen& scr, bool victory) {
    int cy = scr.rows() / 2;
    const char* filename = victory ? "victoryTitle.txt" : "gameOverTitle.txt";
    std::ifstream f(assetsDir() + filename);
    std::string line;
    int ty = cy - 8;
    Color color = victory ? COL_YELLOW : COL_RED;
    while (std::getline(f, line)) {
        int cols = 0;
        for (unsigned char c : line)
            if ((c & 0xC0) != 0x80) cols++;
        int x = scr.cols() / 2 - cols / 2;
        scr.putStr(x, ty++, line, color, COL_BLACK, CELL_BOLD);
    }
    if (victory) {
        drawCentered(scr, ty + 1, 0, scr.cols(), "Gracias por jugar Tenebrarium.", COL_CYAN, 0);
        drawCentered(scr, ty + 2, 0, scr.cols(), "~ Créditos ~", COL_WHITE, CELL_BOLD);
        drawCentered(scr, ty + 3, 0, scr.cols(), "Halley & Nato Co.", COL_YELLOW, 0);
    }
    drawCentered(scr, scr.rows() - 3, 0, scr.cols(),
                 "ENTER para volver al menu",
                 COL_GRAY, CELL_DIM);
}

// ─── drawQuitDialog ───────────────────────────────────────────────────────────

void Renderer::drawQuitDialog(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    int w = 36, h = 7;
    int bx = cx - w / 2, by = cy - h / 2;
    drawBorder(scr, bx, by, w, h, COL_YELLOW);
    drawCentered(scr, by + 1, bx, w, "Salir del juego", COL_WHITE, CELL_BOLD);
    drawCentered(scr, by + 2, bx, w, "¿Qué deseas hacer?", COL_GRAY, CELL_DIM);
    const char* opts[] = { "Menú Principal", "Salir al escritorio" };
    for (int i = 0; i < 2; i++) {
        bool sel = (i == selection);
        std::string label = std::string("  ") + opts[i] + "  ";
        Color c = sel ? COL_YELLOW : COL_WHITE;
        uint8_t fl = sel ? (CELL_BOLD | CELL_INVERTED) : 0;
        drawCentered(scr, by + 4 + i, bx, w, label, c, fl);
    }
    drawCentered(scr, by + h - 1, bx, w, "ESC para cancelar", COL_GRAY, CELL_DIM);
}

// ─── drawShop ─────────────────────────────────────────────────────────────────

void Renderer::drawShop(TerminalScreen& scr, const std::vector<ShopItem>& stock,
                         int selection, const Player& player,
                         const std::string& message,
                         bool sellMode, int sellSelection) {
    int cx = scr.cols() / 2;

    if (sellMode) {
        int w = 66, sc2 = cx - w / 2, r = 2;
        auto& inv = player.getInventory().items();
        int boxH = static_cast<int>(inv.size()) + 8;
        drawBorder(scr, sc2, 1, w, boxH, COL_ORANGE);
        scr.putStr(sc2 + 2, r++, "=== VENDER ===", COL_ORANGE, COL_BLACK, CELL_BOLD);
        scr.putStr(sc2 + 2, r++,
                   "$ " + std::to_string(player.getCoins()) + " monedas disponibles",
                   COL_ORANGE);
        drawHSep(scr, sc2 + 1, r++, w - 2, COL_ORANGE);
        bool any = false;
        for (int i = 0; i < static_cast<int>(inv.size()); i++) {
            const auto& item = inv[i];
            if (item.type != ItemType::Weapon && item.type != ItemType::Armor)
                continue;
            any = true;
            bool sel = (i == sellSelection);
            int sellPrice = item.value / 2;
            std::string tag = (item.type == ItemType::Weapon) ? "[ARMA] " : "[ARMADURA]";
            std::string line = tag + " " + item.name + "  -  " +
                               std::to_string(sellPrice) + " $";
            Color lc = sel ? COL_ORANGE : COL_WHITE;
            uint8_t lf = sel ? CELL_INVERTED : 0;
            scr.putStr(sc2 + 2, r++, line, lc, COL_BLACK, lf);
        }
        if (!any)
            scr.putStr(sc2 + 2, r++, "(nada que vender)", COL_GRAY, COL_BLACK, CELL_DIM);
        drawHSep(scr, sc2 + 1, r++, w - 2, COL_ORANGE);
        if (!message.empty())
            scr.putStr(sc2 + 2, r++, message, COL_GREEN, COL_BLACK, CELL_BOLD);
        scr.putStr(sc2 + 2, r,
                   "W/S navegar  |  ENTER vender  |  V comprar  |  ESC salir",
                   COL_GRAY, COL_BLACK, CELL_DIM);
        return;
    }

    int w = 66, sc2 = cx - w / 2, r = 2;
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
               "W/S navegar  |  ENTER comprar  |  V vender  |  ESC salir",
               COL_GRAY, COL_BLACK, CELL_DIM);
}
