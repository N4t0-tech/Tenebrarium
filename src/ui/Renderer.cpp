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
static constexpr Color COL_GREEN   = {  60, 230, 110, 255 };
static constexpr Color COL_RED     = { 255,  80,  80, 255 };
static constexpr Color COL_GRAY    = { 200, 200, 200, 255 };
static constexpr Color COL_ORANGE  = { 255, 185,  40, 255 };
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

// drawMap — renderiza el mapa centrado en el jugador con FOV y gradiente de brillo.
// Cámara: el jugador queda siempre en el centro de la vista (camX, camY son el offset).
// FOV: las celdas visibles se muestran con brillo según distancia al jugador (0.45–1.0).
// Las celdas exploradas pero no visibles se muestran en gris oscuro (COL_DARK).
// Las paredes secretas no exploradas/visibles se omiten para no revelarlas.
void Renderer::drawMap(TerminalScreen& scr, int col, int row,
                       int viewW, int viewH,
                       const Map& map, const std::vector<MapEntity>& entities) {
    Position pp = map.getPlayerPos();
    int camX = pp.x - viewW / 2;
    int camY = pp.y - viewH / 2;

    auto applyFactor = [](Color c, float f) -> Color {
        return { (uint8_t)(c.r * f), (uint8_t)(c.g * f),
                 (uint8_t)(c.b * f), c.a };
    };

    static constexpr float FOV_RADIUS = 8.0f;  // radio del FOV en celdas

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
                float dx = (float)(mx - pp.x), dy = (float)(my - pp.y);
                float dist  = std::sqrt(dx*dx + dy*dy);
                float factor = std::max(0.45f, 1.0f - (dist / FOV_RADIUS) * 0.55f);

                bool drew = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        Color ec = applyFactor(colorFromPair(ent.colorPair), factor);
                        scr.put(dc, dr, ent.glyph, ec, COL_BLACK,
                                ent.bold ? CELL_BOLD : 0);
                        drew = true;
                        break;
                    }
                }
                if (drew) continue;

                if (tile.type == TileType::SecretWall) {
                    scr.put(dc, dr, '#', applyFactor(COL_GRAY, factor), COL_BLACK, CELL_DIM);
                    continue;
                }
                scr.put(dc, dr, tile.glyph, applyFactor(COL_WHITE, factor), COL_BLACK, 0);
            } else {
                static const Color COL_DARK = { 80, 80, 80, 255 };
                // La pared secreta usa '#' (como pared normal) para no parpadear al salir del FOV
                char32_t g = (tile.type == TileType::SecretWall) ? '#' : tile.glyph;
                scr.put(dc, dr, g, COL_DARK, COL_BLACK, 0);
            }
        }
    }
}

// ─── drawPortrait ─────────────────────────────────────────────────────────────

void Renderer::drawPortrait(TerminalScreen& scr, int col, int row,
                             const char* filename, float scale) {
    if (!filename) {
        scr.putStr(col + 1, row + 1, "[sin retrato]", COL_GRAY, COL_BLACK, CELL_DIM);
        return;
    }
    try {
        XpFile xp = loadXp(assetsDir() + "art/" + filename);
        if (xp.layers.empty()) throw std::runtime_error("vacio");
        xpDrawHalfBlock(scr, xp.layers[0], col, row, scale);
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
    put("+ " + std::to_string(player.countConsumables()) + " pocion(es)", COL_GREEN);
    int fl = player.getDungeonFloor();
    std::string diff = fl <= 2 ? "Facil" : fl <= 4 ? "Normal" : fl <= 6 ? "Dificil" : "Peligroso";
    Color dc = fl <= 2 ? COL_GREEN : fl <= 4 ? COL_YELLOW : COL_RED;
    put("Piso " + std::to_string(fl) + " [" + diff + "]", dc, CELL_DIM);
    drawHSep(scr, col, r++, 20);
    put("WASD  mover",    COL_GRAY, CELL_DIM);
    put("  P   pocion",   COL_GRAY, CELL_DIM);
    put("  I   mochila",  COL_GRAY, CELL_DIM);
    put("  M   misiones", COL_GRAY, CELL_DIM);
    put("  Q   salir",    COL_GRAY, CELL_DIM);
    put("+/-   zoom " + std::to_string(mapZoom) + "x", COL_GRAY, CELL_DIM);
}

void Renderer::drawHudBar(TerminalScreen& scr, int row, const Player& player, int mapZoom) {
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
    b("  +" + std::to_string(player.countConsumables()), COL_GREEN);
    b("  WASD:mover  P:pocion  I:mochila  M:misiones  Q:salir"
      "  +/-:zoom(" + std::to_string(mapZoom) + "x)", COL_GRAY, CELL_DIM);
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
        scr.putStr(x, ty++, line, COL_YELLOW, COL_BLACK, CELL_BOLD);
    }
    ty++;
    scr.putStr(cx - 16, ty++, "~ Un RPG de mazmorra y sombras ~", COL_WHITE);
    ty++;
    const char* opts3[] = { "Continuar", "Nueva Partida", "Creditos", "Salir" };
    const char* opts2[] = { "Nueva Partida", "Creditos", "Salir" };
    int n = hasSave ? 4 : 3;
    const char** opts = hasSave ? opts3 : opts2;
    for (int i = 0; i < n; i++) {
        std::string label = std::string("  ") + opts[i] + "  ";
        int x = cx - static_cast<int>(label.size()) / 2;
        bool sel = (i == selection);
        Color c = sel ? COL_YELLOW : COL_WHITE;
        uint8_t fl = sel ? CELL_BOLD : 0;
        // El item seleccionado parpadea entre invertido y normal
        if (sel && blink) fl |= CELL_INVERTED;
        scr.putStr(x, ty++, label, c, COL_BLACK, fl);
    }
    ty++;
    drawCentered(scr, ty, 0, scr.cols(),
                 "Flechas para navegar  |  ENTER para confirmar",
                 COL_GRAY, CELL_DIM);
}

// ─── drawCredits ─────────────────────────────────────────────────────────────

void Renderer::drawCredits(TerminalScreen& scr) {
    int cx = scr.cols() / 2;
    int cy = scr.rows() / 2;

    drawCentered(scr, cy - 8, 0, scr.cols(), "T E N E B R A R I U M", COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 6, 0, scr.cols(), "~ Creditos ~", COL_WHITE, CELL_BOLD);

    int y = cy - 4;
    drawCentered(scr, y++, 0, scr.cols(), "Halley & Nato Co.", COL_WHITE);
    y++;
    drawCentered(scr, y,   0, scr.cols(), "ESC para volver", COL_GRAY, CELL_DIM);
}

// ─── drawNameInput ────────────────────────────────────────────────────────────

void Renderer::drawNameInput(TerminalScreen& scr, const std::string& name, bool blink) {
    int cy = scr.rows() / 2;
    drawCentered(scr, cy - 4, 0, scr.cols(), "T E N E B R A R I U M",
                 COL_CYAN, CELL_BOLD);
    drawCentered(scr, cy - 2, 0, scr.cols(), "Ingresa el nombre de tu personaje:", COL_WHITE);
    std::string display = "> " + name + (blink ? "_" : " ");
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
// Para añadir una clase: agregar entrada aquí y crear el archivo .xp en assets/art/.
// portraitPath == nullptr muestra "[sin retrato]" como placeholder.
static constexpr ClassInfo kClasses[3] = {
    {"GUERRERO","Vanguardia","Maestro del combate cuerpo a cuerpo.",
     "warrior.xp", 120,15,8,20},
    {"MAGO","Arcano","Domina las artes magicas. Poder devastador.",
     "mago.xp", 70,8,3,100},
    {"RANGER","Explorador","Agil y versatil. Experto en trampas y arco.",
     nullptr, 90,12,5,50},
};

void Renderer::drawClassSelect(TerminalScreen& scr, int selection) {
    drawCentered(scr, 1, 0, scr.cols(), "T E N E B R A R I U M", COL_CYAN, CELL_BOLD);
    drawCentered(scr, 2, 0, scr.cols(), "Elige tu clase:", COL_WHITE);

    // Calcular boxW según el contenido más ancho de cualquier clase
    int maxContent = 0;
    for (int i = 0; i < 3; i++) {
        const auto& c = kClasses[i];
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
    int boxW = maxContent + 4;  // 2 padding + 2 borde
    int boxH = 8, listCol = 2;

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

    // Centrar el retrato escalado en el espacio entre la lista y el borde derecho
    static constexpr float kPortScale = 0.95f;
    static constexpr int kSrcSize = 60;
    int portW = static_cast<int>(kSrcSize * kPortScale);
    int portH = static_cast<int>(kSrcSize * kPortScale / 2);  // half-block divide alto entre 2
    int spaceStart = listCol + boxW + 2;
    int portCol = spaceStart + (scr.cols() - spaceStart - portW) / 2;
    if (portCol < spaceStart) portCol = spaceStart;
    drawBorder(scr, portCol - 1, 3, portW + 2, portH + 2, COL_YELLOW);
    drawPortrait(scr, portCol, 4, kClasses[selection].portrait, kPortScale);

    drawCentered(scr, scr.rows() - 2, 0, scr.cols(),
                 "Arriba/Abajo navegar  |  ENTER confirmar  |  ESC volver",
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
                 "Izq/Der navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawExploration ─────────────────────────────────────────────────────────

void Renderer::drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::string& message, int mapZoom) {
    if (layout == HudLayout::Sidebar) {
        int panelW = 22;
        int mapW   = scr.cols() - panelW - 1;
        drawMap(scr, 0, 0, mapW, scr.rows(), map, entities);
        drawVSep(scr, mapW, 0, scr.rows());
        drawHudPanel(scr, mapW + 1, 0, player, mapZoom);
        if (!message.empty())
            drawCentered(scr, scr.rows() / 2, 0, mapW,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    } else {
        int hudH = 3;
        int mapH = scr.rows() - hudH;
        drawMap(scr, 0, 0, scr.cols(), mapH, map, entities);
        drawHudBar(scr, mapH, player, mapZoom);
        if (!message.empty())
            drawCentered(scr, mapH / 2, 0, scr.cols(),
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED);
    }
}

static Color hpColor(int hp, int maxHp) {
    float pct = (maxHp > 0) ? static_cast<float>(hp) / maxHp : 0.f;
    if (pct > 0.5f) return COL_GREEN;
    if (pct > 0.25f) return COL_YELLOW;
    return COL_RED;
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
    Color phpc = hpColor(player.getHp(), player.getMaxHp());
    scr.putStr(20, br, "HP:", phpc, COL_BLACK, CELL_BOLD);
    scr.putStr(24, br, std::to_string(player.getHp()) + "/" +
               std::to_string(player.getMaxHp()), phpc);
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
        int pots = player.countConsumables();
        std::string uLabel = "[U] Usar pocion (" + std::to_string(pots) + ")";
        scr.putStr(1,        br,   uLabel, pots > 0 ? COL_CYAN : COL_GRAY);
        scr.putStr(cols / 2, br++, "[TAB] Cambiar objetivo", COL_GRAY, COL_BLACK, CELL_DIM);
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

void Renderer::drawInventory(TerminalScreen& scr, const Player& player,
                              int selection, int tab, const std::string& msg) {
    const int w   = std::min(scr.cols() - 4, 64);
    const int x0  = (scr.cols() - w) / 2;
    const int y0  = 1;
    const int h   = scr.rows() - 2;
    drawBorder(scr, x0, y0, w, h);

    // ── Pestañas ──────────────────────────────────────────────────────────────
    const char* tabs[2] = { " POCIONES ", " EQUIPO " };
    int tx = x0 + 2;
    for (int t = 0; t < 2; t++) {
        bool active = (t == tab);
        scr.putStr(tx, y0, tabs[t],
                   active ? COL_YELLOW : COL_GRAY,
                   COL_BLACK,
                   active ? CELL_BOLD | CELL_INVERTED : CELL_DIM);
        tx += static_cast<int>(std::string(tabs[t]).size()) + 1;
    }
    scr.putStr(tx, y0, " ←/→ cambiar ", COL_GRAY, COL_BLACK, CELL_DIM);

    // Vista filtrada+ordenada por statBonus desc
    const auto& allItems = player.getInventory().items();
    std::vector<int> idx;
    for (int i = 0; i < static_cast<int>(allItems.size()); i++) {
        bool isConsumable = (allItems[i].type == ItemType::Consumable);
        if (tab == 0 ? isConsumable : !isConsumable)
            idx.push_back(i);
    }
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return allItems[a].statBonus > allItems[b].statBonus;
    });

    int r = y0 + 2;
    const int contentBottom = y0 + h - 4;  // reservar 3 filas para descripción + controles

    if (tab == 0) {
        // ── Pestaña Pociones ──────────────────────────────────────────────────
        // Barra de HP
        int hp = player.getHp(), maxHp = player.getMaxHp();
        bool full = (hp >= maxHp);
        Color hpCol = full ? COL_GREEN : hpColor(hp, maxHp);
        std::string hpLabel = "HP " + std::to_string(hp) + "/" + std::to_string(maxHp);
        if (full) hpLabel += "  *** LLENO ***";
        scr.putStr(x0 + 2, r, hpLabel, hpCol, COL_BLACK, full ? CELL_BOLD : 0);
        r++;
        drawStatBar(scr, x0 + 2, r++, hp, maxHp, w - 4, hpCol);
        drawHSep(scr, x0 + 1, r++, w - 2);

        if (idx.empty()) {
            scr.putStr(x0 + 2, r, "(sin pociones)", COL_GRAY, COL_BLACK, CELL_DIM);
        } else {
            for (int i = 0; i < static_cast<int>(idx.size()) && r < contentBottom; i++) {
                const auto& item = allItems[idx[i]];
                bool sel = (selection == i);
                std::string line = "[POCION]  " + item.name + "  +" + std::to_string(item.statBonus) + " HP";
                scr.putStr(x0 + 2, r++, line,
                           sel ? COL_WHITE : COL_CYAN,
                           COL_BLACK,
                           sel ? CELL_INVERTED : 0);
            }
        }
    } else {
        // ── Pestaña Equipo ────────────────────────────────────────────────────
        // Slots equipados
        auto drawSlot = [&](int sel, const char* label, const std::optional<Item>& slot, bool isWeapon) {
            bool active = (selection == sel);
            std::string line;
            Color c;
            if (slot) {
                line = std::string(label) + ": " + slot->name +
                       "  +" + std::to_string(slot->statBonus) +
                       (isWeapon ? " ATK" : " DEF") + "  [E=desequipar]";
                c = isWeapon ? COL_YELLOW : COL_GREEN;
            } else {
                line = std::string(label) + ": (vacio)";
                c = COL_GRAY;
            }
            scr.putStr(x0 + 2, r, line, c, COL_BLACK,
                       active ? CELL_INVERTED : (slot ? 0 : CELL_DIM));
        };
        drawSlot(0, "Arma    ", player.getEquippedWeapon(), true);  r++;
        drawSlot(1, "Armadura", player.getEquippedArmor(),  false); r++;
        drawHSep(scr, x0 + 1, r++, w - 2);

        if (idx.empty()) {
            scr.putStr(x0 + 2, r, "(mochila vacia)", COL_GRAY, COL_BLACK, CELL_DIM);
        } else {
            for (int i = 0; i < static_cast<int>(idx.size()) && r < contentBottom; i++) {
                const auto& item = allItems[idx[i]];
                bool sel = (selection == 2 + i);
                bool isWeapon = (item.type == ItemType::Weapon);
                std::string tag  = isWeapon ? "[ARMA]    " : "[ARMADURA]";
                std::string stat = isWeapon ? " ATK" : " DEF";
                std::string line = tag + " " + item.name + "  +" + std::to_string(item.statBonus) + stat;

                // Diferencia vs equipado
                const auto& equipped = isWeapon ? player.getEquippedWeapon() : player.getEquippedArmor();
                std::string diffStr;
                Color diffCol = COL_WHITE;
                if (equipped) {
                    int d = item.statBonus - equipped->statBonus;
                    if (d > 0)      { diffStr = "  ↑+" + std::to_string(d); diffCol = COL_GREEN;  }
                    else if (d < 0) { diffStr = "  ↓"  + std::to_string(d); diffCol = COL_RED;    }
                    else            { diffStr = "  =";                       diffCol = COL_GRAY;   }
                }

                Color baseCol = isWeapon ? COL_YELLOW : COL_GREEN;
                scr.putStr(x0 + 2, r, line, sel ? COL_WHITE : baseCol, COL_BLACK,
                           sel ? CELL_INVERTED : 0);
                if (!diffStr.empty())
                    scr.putStr(x0 + 2 + static_cast<int>(line.size()), r, diffStr,
                               diffCol, COL_BLACK, sel ? CELL_INVERTED : 0);
                r++;
            }
        }
    }

    // ── Panel inferior: descripción + mensaje + controles ─────────────────────
    drawHSep(scr, x0 + 1, contentBottom, w - 2);

    // Descripción del ítem seleccionado
    std::string desc;
    if (tab == 0 && !idx.empty() && selection < static_cast<int>(idx.size())) {
        desc = allItems[idx[selection]].description;
    } else if (tab == 1) {
        if (selection == 0 && player.getEquippedWeapon())
            desc = player.getEquippedWeapon()->description;
        else if (selection == 1 && player.getEquippedArmor())
            desc = player.getEquippedArmor()->description;
        else if (selection >= 2 && (selection - 2) < static_cast<int>(idx.size()))
            desc = allItems[idx[selection - 2]].description;
    }

    if (!msg.empty())
        scr.putStr(x0 + 2, contentBottom + 1, msg, COL_YELLOW, COL_BLACK, CELL_BOLD);
    else if (!desc.empty())
        scr.putStr(x0 + 2, contentBottom + 1, desc.substr(0, w - 4), COL_GRAY, COL_BLACK, CELL_DIM);

    const char* hint = (tab == 0)
        ? "[↑↓] Nav  [E/U] Usar  [ESC] Cerrar"
        : "[↑↓] Nav  [E] Equipar/Desequipar  [ESC] Cerrar";
    scr.putStr(x0 + 2, contentBottom + 2, hint, COL_GRAY, COL_BLACK, CELL_DIM);
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
                 " Arriba/Abajo navegar  |  ESC volver ", COL_GRAY, CELL_DIM);
}

// ─── drawGameOver ─────────────────────────────────────────────────────────────

void Renderer::drawGameOver(TerminalScreen& scr, bool victory) {
    int cy = scr.rows() / 2;
    if (victory) {
        drawCentered(scr, cy - 5, 0, scr.cols(), "* * * V I C T O R I A * * *",
                     COL_YELLOW, CELL_BOLD);
        drawCentered(scr, cy - 3, 0, scr.cols(), "Has conquistado la mazmorra.",
                     COL_WHITE, CELL_BOLD);
        drawCentered(scr, cy - 2, 0, scr.cols(), "Gracias por jugar Tenebrarium.",
                     COL_CYAN, 0);
        drawCentered(scr, cy,     0, scr.cols(), "~ Creditos ~", COL_WHITE, CELL_BOLD);
        drawCentered(scr, cy + 1, 0, scr.cols(), "Halley & Nato Co.", COL_YELLOW, 0);
    } else {
        drawCentered(scr, cy - 1, 0, scr.cols(), "G A M E   O V E R",
                     COL_RED, CELL_BOLD);
    }
    drawCentered(scr, cy + 3, 0, scr.cols(), "ENTER para volver al menu",
                 COL_GRAY, CELL_DIM);
}

// ─── drawQuitDialog ───────────────────────────────────────────────────────────

void Renderer::drawQuitDialog(TerminalScreen& scr, int selection) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;
    int w = 36, h = 7;
    int bx = cx - w / 2, by = cy - h / 2;
    drawBorder(scr, bx, by, w, h, COL_YELLOW);
    drawCentered(scr, by + 1, bx, w, "Salir del juego", COL_WHITE, CELL_BOLD);
    drawCentered(scr, by + 2, bx, w, "Que deseas hacer?", COL_GRAY, CELL_DIM);
    const char* opts[] = { "Menu Principal", "Salir al escritorio" };
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
