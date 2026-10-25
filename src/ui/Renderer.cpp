#include "Renderer.hpp"
#include "combat/MapSpells.hpp"
#include "ui/XpLoader.hpp"
#include "core/Assets.hpp"
#include "entities/SkillTree.hpp"
#include "Version.hpp"
#include <raylib.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cstdio>

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
static constexpr Color COL_TORCH   = { 255, 190, 120, 255 };
static constexpr Color COL_BROWN   = { 169, 112,  52, 255 };

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

static Color colorForTier(int tier);

static Color shopItemColor(const Item& item) {
    bool gear = item.type == ItemType::Weapon || item.type == ItemType::Armor;
    return gear ? colorForTier(item.tier) : COL_WHITE;
}

static Color colorForTier(int tier) {
    switch (tier) {
        case 2:  return COL_BLUE;
        case 3:  return COL_MAGENTA;
        case 4:  return COL_ORANGE;
        default: return COL_GRAY;
    }
}

const char* Renderer::className(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior:    return "Guerrero";
        case PlayerClass::Mage:       return "Mago";
        case PlayerClass::Ranger:     return "Arquera";
        case PlayerClass::Halley:     return "Halley";
        case PlayerClass::Nato:       return "Nato";
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

static int utf8Len(const std::string& s) {
    int len = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.c_str());
    while (*p) {
        len++;
        if      (*p < 0x80) p += 1;
        else if (*p < 0xE0) p += 2;
        else if (*p < 0xF0) p += 3;
        else                p += 4;
    }
    return len;
}

static std::string utf8Substr(const std::string& s, int startCp, int countCp) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.c_str());
    int idx = 0;
    while (*p && idx < startCp) {
        if      (*p < 0x80) p += 1;
        else if (*p < 0xE0) p += 2;
        else if (*p < 0xF0) p += 3;
        else                p += 4;
        idx++;
    }
    const unsigned char* begin = p;
    int taken = 0;
    while (*p && taken < countCp) {
        int len;
        if      (*p < 0x80) len = 1;
        else if (*p < 0xE0) len = 2;
        else if (*p < 0xF0) len = 3;
        else                len = 4;
        p += len;
        taken++;
    }
    return std::string(reinterpret_cast<const char*>(begin), p - begin);
}

static std::string scrollText(const std::string& text, int maxWidth, int tick, int speed = 5) {
    int nameLen = utf8Len(text);
    if (nameLen <= maxWidth) return text;
    int gap = 3;
    int cycle = nameLen + gap + maxWidth;
    int pos = (tick / speed) % cycle;

    if (pos < nameLen) {
        int visible = std::min(nameLen - pos, maxWidth);
        std::string result = utf8Substr(text, pos, visible);
        result.append(maxWidth - visible, ' ');
        return result;
    }
    pos -= nameLen;
    if (pos < gap)
        return std::string(maxWidth, ' ');
    pos -= gap;
    return std::string(maxWidth - pos, ' ') + utf8Substr(text, 0, pos);
}

// Parte el texto en líneas de como máximo `width` columnas sin cortar palabras
// (salvo que una palabra sola no quepa).
static std::vector<std::string> wrapText(const std::string& text, int width) {
    std::vector<std::string> out;
    if (width <= 0) return out;
    std::string cur;
    size_t i = 0;
    while (i < text.size()) {
        size_t sp = text.find(' ', i);
        std::string word = text.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
        i = (sp == std::string::npos) ? text.size() : sp + 1;
        while (utf8Len(word) > width) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            out.push_back(utf8Substr(word, 0, width));
            word = utf8Substr(word, width, utf8Len(word) - width);
        }
        if (cur.empty())                                    cur = word;
        else if (utf8Len(cur) + 1 + utf8Len(word) <= width) cur += " " + word;
        else { out.push_back(cur); cur = word; }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Recorta a `width` columnas marcando el corte con '~'.
static std::string fitText(const std::string& s, int width) {
    if (width <= 0) return "";
    return utf8Len(s) > width ? utf8Substr(s, 0, width - 1) + "~" : s;
}

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
    int len = utf8Len(str);
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
        case PlayerClass::Warrior:    return COL_YELLOW;
        case PlayerClass::Mage:       return COL_CYAN;
        case PlayerClass::Ranger:     return COL_GREEN;
        case PlayerClass::Halley:     return COL_MAGENTA;
        case PlayerClass::Nato:       return COL_RED;
    }
    return COL_YELLOW;
}

static const char* portraitForClass(PlayerClass pc) {
    switch (pc) {
        case PlayerClass::Warrior:    return "mini-warrior.xp";
        case PlayerClass::Mage:       return "mini-wizard.xp";
        case PlayerClass::Ranger:     return "mini-ranger.xp";
        case PlayerClass::Halley:     return "mini-halley.xp";
        case PlayerClass::Nato:       return "mini-nato.xp";
    }
    return "mini-warrior.xp";
}

// ─── drawMap ─────────────────────────────────────────────────────────────────

void Renderer::drawMap(TerminalScreen& scr, int col, int row,
                       int viewW, int viewH,
                       const Map& map, const std::vector<MapEntity>& entities,
                       const std::vector<Position>& torches,
                       Color playerColor, int floor, bool isVillage, const DayLight& light,
                       std::optional<Position> cam) {
    Position pp = map.getPlayerPos();
    int camX = cam ? cam->x : pp.x - viewW / 2;
    int camY = cam ? cam->y : pp.y - viewH / 2;

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
    Color colFloor, colWall, colTile, colDim, colDimGlyph;
    if (isVillage) {
        colFloor     = BLACK;               // fondo negro del pueblo
        colWall      = BLACK;
        colTile      = { 190, 168, 116, 255 };
        colDim       = BLACK;
        colDimGlyph  = { 95,  84,  58, 255 };
    } else {
        colFloor   = lerp(FG_GREEN, FG_MONO, t);
        colWall    = lerp(WG_GREEN, WG_MONO, t);
        colDim     = lerp(DG_GREEN, DG_MONO, t);
        colTile    = lerp(TILE_GREEN, TILE_MONO, t);
        colDimGlyph = lerp(DIM_GREEN, DIM_MONO, t);
    }

    static constexpr float FOV_RADIUS = 8.0f;
    float cellAspect = (float)scr.cellH() / scr.cellW();

    const float darkness = isVillage ? light.darkness : 0.0f;
    auto nightTint = [&light, isVillage](Color c) -> Color {
        if (!isVillage) return c;
        float gray = 0.30f * c.r + 0.59f * c.g + 0.11f * c.b;
        auto ch = [&](uint8_t v, float mul, float add) {
            float d = v + (gray - v) * light.desat;
            return (uint8_t)std::clamp(d * mul + add, 0.0f, 255.0f);
        };
        return { ch(c.r, light.mulR, light.addR), ch(c.g, light.mulG, light.addG),
                 ch(c.b, light.mulB, light.addB), c.a };
    };

    // Rellenar todo el viewport con negro frío
    for (int sy = 0; sy < viewH; sy++)
        for (int sx = 0; sx < viewW; sx++)
            scr.put(col + sx, row + sy, ' ', COL_BLACK, COL_COLD_BG, 0);

    for (int sy = 0; sy < viewH; sy++) {
        for (int sx = 0; sx < viewW; sx++) {
            int mx = camX + sx, my = camY + sy;
            int dc = col + sx, dr = row + sy;
            if (mx < 0 || mx >= map.width() || my < 0 || my >= map.height()) continue;
            auto cur = std::find_if(entities.begin(), entities.end(), [&](const MapEntity& e) {
                return e.cursor && e.pos.x == mx && e.pos.y == my;
            });
            if (cur != entities.end()) {
                scr.put(dc, dr, cur->glyph, colorFromPair(cur->colorPair), COL_BLACK,
                        CELL_BOLD | CELL_INVERTED);
                continue;
            }
            const Tile& tile = map.at(mx, my);
            if (!tile.explored) continue;

            if (tile.visible) {
                float dx = (float)(mx - pp.x), dy = (float)(my - pp.y);
                float pixelDist = std::sqrt(dx*dx + dy*dy * cellAspect * cellAspect);
                float baseFactor = std::max(0.45f, 1.0f - (pixelDist / FOV_RADIUS) * 0.55f);
                float factor = isVillage ? (1.0f - darkness) + darkness * baseFactor : baseFactor;

                // Luz dinámica de antorchas
                float torchBoost = 0.0f;
                float torchRadius = isVillage ? 8.0f : 5.0f;
                float torchIntensity = isVillage ? 0.25f : 0.15f;
                if (!torches.empty()) {
                    auto hasLos = [&](int x1, int y1, int x2, int y2) -> bool {
                        int dx = x2 - x1, dy = y2 - y1;
                        int adx = std::abs(dx), ady = std::abs(dy);
                        int sx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
                        int sy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
                        int err = adx - ady;
                        int x = x1, y = y1;
                        while (x != x2 || y != y2) {
                            int e2 = 2 * err;
                            if (e2 > -ady) { err -= ady; x += sx; }
                            if (e2 <  adx) { err += adx; y += sy; }
                            if (x == x2 && y == y2) break;
                            if (!map.isWalkable(x, y)) return false;
                        }
                        return true;
                    };

                    for (size_t ti = 0; ti < torches.size(); ti++) {
                        if (!hasLos(torches[ti].x, torches[ti].y, mx, my)) continue;
                        float tdx = (float)(mx - torches[ti].x);
                        float tdy = (float)(my - torches[ti].y);
                        float tdist = std::sqrt(tdx*tdx + tdy*tdy);
                        float falloff = std::max(0.0f, 1.0f - tdist / torchRadius);
                        if (falloff > 0.0f) {
                            float flicker = 0.85f + 0.15f * std::sin(GetTime() * 3.0f + ti * 5.7f);
                            torchBoost += falloff * flicker;
                        }
                    }
                }
                float effectiveTorch = isVillage ? torchBoost * darkness : torchBoost;
                float f = std::min(1.0f, factor + effectiveTorch * torchIntensity);

                auto litBg = [&](Color c) -> Color {
                    if (effectiveTorch > 0) {
                        float w = std::min(1.0f, effectiveTorch * torchIntensity);
                        return { (uint8_t)(c.r * (1-w) + COL_TORCH.r * w),
                                 (uint8_t)(c.g * (1-w) + COL_TORCH.g * w),
                                 (uint8_t)(c.b * (1-w) + COL_TORCH.b * w), 255 };
                    }
                    return c;
                };

                // Color que se ve de la casilla del pueblo: el glifo si es un
                // bloque lleno, el fondo en otro caso. Las entidades lo usan de fondo.
                auto villageGround = [&]() -> Color {
                    Color g = (tile.glyph == 0x2588)
                        ? Color{tile.fg_r, tile.fg_g, tile.fg_b, 255}
                        : Color{tile.bg_r, tile.bg_g, tile.bg_b, 255};
                    return litBg(applyFactor(nightTint(g), f));
                };

                // Jugador (dentro del bloque visible para que reciba torchBoost)
                if (mx == pp.x && my == pp.y) {
                    Color entBg = isVillage ? villageGround() : litBg(colFloor);
                    scr.put(dc, dr, '@', applyFactor(playerColor, f), entBg, CELL_BOLD);
                    continue;
                }

                bool drew = false;
                MapEntity deadFallback{};
                bool hasDeadFallback = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        if (ent.dead) {
                            if (!hasDeadFallback) { deadFallback = ent; hasDeadFallback = true; }
                            continue;
                        }
                        Color ec = applyFactor(colorFromPair(ent.colorPair), f);
                        Color entBg = isVillage ? villageGround() : litBg(colFloor);
                        scr.put(dc, dr, ent.glyph, ec, entBg,
                                ent.bold ? CELL_BOLD : 0);
                        drew = true;
                        break;
                    }
                }
                if (!drew && hasDeadFallback) {
                    Color ec = applyFactor(COL_GRAY, f * 0.85f);
                    Color entBg = isVillage ? villageGround() : litBg(colFloor);
                    scr.put(dc, dr, deadFallback.glyph, ec, entBg,
                            CELL_DIM | CELL_STRIKETHROUGH);
                    drew = true;
                }
                if (drew) continue;

                if (tile.type == TileType::SecretWall) {
                    scr.put(dc, dr, '#', applyFactor(COL_GRAY, f), colWall, CELL_DIM);
                    continue;
                }
                Color tileBg = (tile.type == TileType::Floor || tile.type == TileType::Stairs) ? colFloor : colWall;
                if (isVillage)
                    tileBg = applyFactor(nightTint(Color{tile.bg_r, tile.bg_g, tile.bg_b, 255}), f);
                Color tileFg = isVillage
                    ? nightTint(Color{tile.fg_r, tile.fg_g, tile.fg_b, 255}) : colTile;
                if (isVillage && effectiveTorch > 0) {
                    float w = std::min(1.0f, effectiveTorch * torchIntensity * 0.4f);
                    tileFg = { (uint8_t)(tileFg.r * (1-w) + COL_TORCH.r * w),
                               (uint8_t)(tileFg.g * (1-w) + COL_TORCH.g * w),
                               (uint8_t)(tileFg.b * (1-w) + COL_TORCH.b * w), 255 };
                }
                scr.put(dc, dr, tile.glyph, applyFactor(tileFg, f),
                        litBg(tileBg), 0);
            } else {
                bool drewAlwaysVisible = false;
                for (const auto& ent : entities) {
                    if (ent.alwaysVisible && ent.pos.x == mx && ent.pos.y == my) {
                        Color ec = ent.dead
                            ? Color{COL_GRAY.r / 2, COL_GRAY.g / 2, COL_GRAY.b / 2, COL_GRAY.a}
                            : Color{(uint8_t)(colorFromPair(ent.colorPair).r / 2),
                                    (uint8_t)(colorFromPair(ent.colorPair).g / 2),
                                    (uint8_t)(colorFromPair(ent.colorPair).b / 2), 255};
                        scr.put(dc, dr, ent.glyph, ec, colDim, ent.dead ? (CELL_DIM | CELL_STRIKETHROUGH) : (ent.bold ? CELL_BOLD : 0));
                        drewAlwaysVisible = true;
                        break;
                    }
                }
                if (drewAlwaysVisible) continue;
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
                             const Player& player, int mapZoom, int scrollTick,
                             bool arrowMovement) {
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

    {
        scr.putStr(col + 1, r, player.getName().substr(0, 14)
            + " [" + className(player) + "] Nv." + std::to_string(player.getLevel()),
            COL_CYAN, COL_BLACK, CELL_BOLD);
        r++;

        std::string xpText = std::to_string(player.getXp()) + "/"
            + std::to_string(player.getXpToNextLevel());
        int xpTextLen = static_cast<int>(xpText.size());
        int barW = sepW - 1 - xpTextLen;
        if (barW >= 2) {
            int xp = player.getXp();
            int xpMax = player.getXpToNextLevel();
            int fill = (xpMax > 0) ? (xp * barW / xpMax) : 0;
            fill = std::max(0, std::min(fill, barW));
            for (int i = 0; i < barW; i++)
                scr.put(col + 1 + i, r, 0x2500,
                        i < fill ? COL_YELLOW : COL_GRAY, COL_BLACK,
                        i < fill ? 0 : CELL_DIM);
            scr.putStr(col + 1 + barW, r, xpText, COL_YELLOW, COL_BLACK, 0);
        } else {
            scr.putStr(col + 1, r, xpText, COL_YELLOW, COL_BLACK, 0);
        }
        r++;
    }
    drawHSep(scr, col, r++, sepW);
    put("HP " + std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp()), COL_GREEN);
    drawStatBar(scr, col + 1, r++, player.getHp(), player.getMaxHp(), 14, COL_GREEN);
    std::string mpLabel = player.getClass() == PlayerClass::Warrior ? "AG " : "MP ";
    put(mpLabel + std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana()), COL_CYAN);
    drawStatBar(scr, col + 1, r++, player.getMana(), player.getMaxMana(), 14, COL_CYAN);
    drawHSep(scr, col, r++, sepW);
    put("ATK " + std::to_string(player.getAttack()), COL_WHITE);
    put("DEF " + std::to_string(player.getDefense()), COL_WHITE);
    drawHSep(scr, col, r++, sepW);
    put("EQUIPO", COL_WHITE, CELL_BOLD);
    int eqW = 17;
    put("Arma    : " + (player.getEquippedWeapon()
        ? scrollText(player.getEquippedWeapon()->name, eqW, scrollTick) : std::string(eqW, '-')),
        player.getEquippedWeapon() ? COL_YELLOW : COL_GRAY,
        player.getEquippedWeapon() ? 0 : CELL_DIM);
    put("Armadura: " + (player.getEquippedArmor()
        ? scrollText(player.getEquippedArmor()->name, eqW, scrollTick) : std::string(eqW, '-')),
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
     put("P " + std::to_string(player.getInventory().getItemCount("Pala")) + " pala(s)", COL_BROWN);
     int fl = player.getDungeonFloor();
    std::string diff = fl <= 2 ? "Fácil" : fl <= 4 ? "Normal" : fl <= 6 ? "Difícil" : "Peligroso";
    Color dc = fl <= 2 ? COL_GREEN : fl <= 4 ? COL_YELLOW : COL_RED;
    std::string incTag = player.getIncursions() > 0
                       ? "  Inc " + std::to_string(player.getIncursions()) : "";
    put("Piso " + std::to_string(fl) + incTag + " [" + diff + "]", dc, CELL_DIM);
    drawHSep(scr, col, r++, sepW);
     put(arrowMovement ? "▲◄▼►  mover" : "WASD  mover", COL_GRAY, CELL_DIM);
     put("  E   bomba",    COL_GRAY, CELL_DIM);
     if (canUseMapAbility(player, ArtEffect::Trampa))
         put("  T   trampa",   COL_GRAY, CELL_DIM);
     if (hasAnyMapSpell(player))
         put("  Q   hechizo",  COL_GRAY, CELL_DIM);
     if (player.getClass() == PlayerClass::Warrior)
         put("  Q   cadáver",  COL_GRAY, CELL_DIM);
      put("  P   poción",   COL_GRAY, CELL_DIM);
     put("  R   tomar",   COL_GRAY, CELL_DIM);
      put("  I   mochila",  COL_GRAY, CELL_DIM);
      put("  M   misiones", COL_GRAY, CELL_DIM);
      put("  B   bestiario",COL_GRAY, CELL_DIM);
      put("  H   habilidades",COL_GRAY, CELL_DIM);
      put("  Esc salir",   COL_GRAY, CELL_DIM);
     put("+/-   zoom " + std::to_string(mapZoom) + "x", COL_GRAY, CELL_DIM);
     put("  F   pantalla",  COL_GRAY, CELL_DIM);
}

// HUD inferior: 6 filas (separador + retrato 20×10 half-block + contenido compacto + controles)
void Renderer::drawHudBar(TerminalScreen& scr, int row, const Player& player, int mapZoom,
                          bool arrowMovement) {
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
    a3(" P:" + std::to_string(player.getInventory().getItemCount("Pala")), COL_BROWN);

    // row 4: floor + zoom
    int c4 = rpC;
    auto a4 = [&](const std::string& s, Color col, uint8_t f = 0) {
        c4 += scr.putStr(c4, row + 4, s, col, COL_BLACK, f);
    };
    int fl = player.getDungeonFloor();
    std::string diff = fl <= 2 ? "Fácil" : fl <= 4 ? "Normal" : fl <= 6 ? "Difícil" : "Peligroso";
    Color dc = fl <= 2 ? COL_GREEN : fl <= 4 ? COL_YELLOW : COL_RED;
    std::string incTag = player.getIncursions() > 0
                       ? "  Inc " + std::to_string(player.getIncursions()) : "";
    a4("Piso " + std::to_string(fl) + incTag + " [" + diff + "]", dc, CELL_DIM);
    a4("  [+/-]zoom:" + std::to_string(mapZoom), COL_GRAY, CELL_DIM);

    // row 5: separator
    for (int x = 0; x < rpW - 2; x++)
        scr.put(rpX + 1 + x, row + 5, 0x2550, COL_WHITE);
    scr.put(rpX, row + 5, 0x2560, COL_WHITE);
    scr.put(rpX + rpW - 1, row + 5, 0x2563, COL_WHITE);

    // row 6-7: controls
    scr.putStr(rpC, row + 6, std::string(arrowMovement ? "[▲◄▼►]Mover" : "[WASD]Mover") + "  [E]Bomba  [R]Tomar  [P]Poc  [I]Inv"
               + (player.getClass() == PlayerClass::Warrior ? "  [Q]Cadáver" : ""),
               COL_GRAY, COL_BLACK, CELL_DIM);
    scr.putStr(rpC, row + 7, std::string("[M]Mis  [B]Best  [H]Hab  [Esc]Salir  [+/-]Zoom  [F]Pantalla")
               + (canUseMapAbility(player, ArtEffect::Trampa) ? "  [T]Trampa" : "")
               + (hasAnyMapSpell(player) ? "  [Q]Hechizo" : ""),
               COL_GRAY, COL_BLACK, CELL_DIM);
}

// ─── drawTitle ───────────────────────────────────────────────────────────────

void Renderer::drawTitle(TerminalScreen& scr) {
    int cx = scr.cols() / 2, cy = scr.rows() / 2;

    // ── Cache title with relative offsets (resize-proof) ──
    struct TitleCell { int codepoint; int colOff; int rowOff; };
    static std::vector<TitleCell> titleCells;
    static bool titleLoaded = false;
    if (!titleLoaded) {
        std::ifstream f(assetsDir() + "title.txt");
        std::string line;
        int rowOff = -8;
        while (std::getline(f, line)) {
            int cols = 0;
            for (unsigned char c : line)
                if ((c & 0xC0) != 0x80) cols++;
            int xOff = -cols / 2;
            const unsigned char* s = reinterpret_cast<const unsigned char*>(line.c_str());
            while (*s) {
                int cp, len;
                if      (*s < 0x80)  { cp = *s;       len = 1; }
                else if (*s < 0xE0)  { cp = (*s & 0x1F) << 6 | (s[1] & 0x3F); len = 2; }
                else if (*s < 0xF0)  { cp = (*s & 0x0F) << 12 | (s[1] & 0x3F) << 6 | (s[2] & 0x3F); len = 3; }
                else                 { cp = (*s & 0x07) << 18 | (s[1] & 0x3F) << 12 | (s[2] & 0x3F) << 6 | (s[3] & 0x3F); len = 4; }
                titleCells.push_back({cp, xOff, rowOff});
                xOff++;
                s += len;
            }
            rowOff++;
        }
        titleLoaded = true;
    }

    // ── Glitch state (offsets tambien en relativo) ──
    struct GlitchInst { int colOff, rowOff; int origCp, glitchCp; double triggerTime; bool active; };
    static constexpr int kGlitchMax = 3;
    static constexpr double kBlockDur   = 0.25;
    static constexpr double kFadeDur    = 0.10;
    static constexpr double kAfterDur   = 0.10;
    static constexpr int kGlyphCount = 10;
    static const int blockGlyphs[] = { 0x2593, 0x2592, 0x2591, 0x2588, '@', '#', '$', '%', '&', '+' };
    static const Color gold = {255, 220, 80, 255};
    static GlitchInst insts[kGlitchMax] = {};
    static double nextGlitchTime = 0.0;
    static int rowGlitchOff = 999;
    static double rowGlitchEnd = 0.0;
    static double nextRowGlitch = 8.0;

    // ── Draw title ──
    for (const auto& cell : titleCells) {
        int screenCol = cx + cell.colOff;
        int screenRow = cy + cell.rowOff;
        bool glitched = false;

        // Point glitch: bloque 250ms → fade 100ms → afterimage 100ms
        for (int i = 0; i < kGlitchMax; i++) {
            auto& g = insts[i];
            if (!g.active || g.colOff != cell.colOff || g.rowOff != cell.rowOff) continue;
            double t = GetTime() - g.triggerTime;
            if (t < kBlockDur) {
                scr.put(screenCol, screenRow, g.glitchCp, gold, COL_BLACK, CELL_BOLD);
            } else if (t < kBlockDur + kFadeDur) {
                float p = (float)((t - kBlockDur) / kFadeDur);
                Color c = { (uint8_t)(255 - (uint8_t)(55 * p)), (uint8_t)(220 - (uint8_t)(20 * p)),
                            (uint8_t)(80 + (uint8_t)(120 * p)), 255 };
                scr.put(screenCol, screenRow, cell.codepoint, c, COL_BLACK, CELL_BOLD);
            } else if (t < kBlockDur + kFadeDur + kAfterDur) {
                scr.put(screenCol, screenRow, cell.codepoint, COL_GRAY, COL_BLACK, CELL_DIM);
            } else {
                g.active = false;
                scr.put(screenCol, screenRow, cell.codepoint, COL_GRAY, COL_BLACK, CELL_BOLD);
            }
            glitched = true;
            break;
        }
        if (glitched) continue;

        // Row glitch: línea completa por 100ms cada ~8s
        if (cell.rowOff == rowGlitchOff && GetTime() < rowGlitchEnd && cell.codepoint != ' ') {
            int gp = blockGlyphs[(cell.colOff * 7 + (int)(GetTime() * 50)) % kGlyphCount];
            scr.put(screenCol, screenRow, gp, gold, COL_BLACK, CELL_BOLD);
            continue;
        }

        scr.put(screenCol, screenRow, cell.codepoint, COL_GRAY, COL_BLACK, CELL_BOLD);
    }

    // ── Update glitch timers ──
    if (GetTime() >= nextGlitchTime && !titleCells.empty()) {
        for (auto& g : insts)
            if (g.active && GetTime() - g.triggerTime >= kBlockDur + kFadeDur + kAfterDur)
                g.active = false;
        int count = GetRandomValue(1, kGlitchMax);
        for (int i = 0, act = 0; i < kGlitchMax && act < count; i++) {
            if (!insts[i].active) {
                int idx = GetRandomValue(0, (int)titleCells.size() - 1);
                if (titleCells[idx].codepoint != ' ') {
                    insts[i] = { titleCells[idx].colOff, titleCells[idx].rowOff,
                                 titleCells[idx].codepoint,
                                 blockGlyphs[GetRandomValue(0, kGlyphCount - 1)],
                                 GetTime(), true };
                    act++;
                }
            }
        }
        nextGlitchTime = GetTime() + 0.8 + GetRandomValue(-200, 300) / 1000.0;
    }

    if (GetTime() >= nextRowGlitch && !titleCells.empty()) {
        rowGlitchOff = titleCells[GetRandomValue(0, (int)titleCells.size() - 1)].rowOff;
        rowGlitchEnd = GetTime() + 0.1;
        nextRowGlitch = GetTime() + 8.0 + GetRandomValue(-2000, 3000) / 1000.0;
    }
    if (GetTime() >= rowGlitchEnd) rowGlitchOff = 999;

    // ── Subtitle ──
    scr.putStr(cx - 16, cy + 1, "~ Un RPG de mazmorra y sombras ~", COL_WHITE);

    std::string version = kGameVersion;
    scr.putStr(scr.cols() - (int)version.size() - 1, scr.rows() - 1, version, COL_DK_GRAY, COL_BLACK, CELL_DIM);
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
    const char* portrait;  // archivo .xp para la pantalla de selección
    int hp, atk, def, mana;
    PlayerClass cls;
};

// Índices 0-2: clases normales (Física, Mágica, Distancia).
// Índices 3-4: clases secretas (Halley, Nato).
static const ClassInfo kClasses[5] = {
    // Física
    {"GUERRERO", "Física",    "Maestro del combate cuerpo a cuerpo.",
     "warrior.xp", 120,20,8,100, PlayerClass::Warrior},
    // Mágica
    {"MAGO",     "Mágica",    "Domina las artes mágicas. Poder devastador.",
     "mago.xp",   100,10,3,100, PlayerClass::Mage},
    // Distancia
    {"ARQUERA",  "Distancia", "Ágil y versátil. Experta en trampas y arco.",
     "ranger.xp",  100,12,5,50,  PlayerClass::Ranger},
    // Secretas
    {"HALLEY",   "Secreta",   "La balanza que todo lo iguala.",
     "halley.xp", 130,18,7,170, PlayerClass::Halley},
    {"NATO",     "Secreta",   "Forjó el Tenebrarium con sus manos.",
     "nato.xp",   140,22,8,130, PlayerClass::Nato},
};

static const char* kCategoryNames[3] = { "Física", "Mágica", "Distancia" };

void Renderer::drawClassSelect(TerminalScreen& scr, int category,
                                bool secretUnlocked) {
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
        // category reused as 0=Halley 1=Nato in secret mode
        int secretSel = category;

        auto drawSecretClass = [&](int kIdx, int x, const char* label, bool sel) {
            Color c = sel ? COL_YELLOW : COL_MAGENTA;
            drawBorder(scr, x - 1, portY - 1, portW + 2, portH + 2, c);
            Color tint = sel ? WHITE : Color{100, 100, 100, 255};
            drawPortrait(scr, x, portY, kClasses[kIdx].portrait, kPortScale, tint);
            drawCentered(scr, portY + portH, x - 1, portW + 2,
                         label, c, sel ? CELL_BOLD : 0);
        };
        drawSecretClass(3, startX,                "HALLEY", secretSel == 0);
        drawSecretClass(4, startX + portW + gap,  "NATO",   secretSel == 1);

        // Ficha de información
        int cardY = portY + portH + 3;
        const auto& ci = kClasses[3 + secretSel];
        std::string stats = "HP:" + std::to_string(ci.hp) +
                            " ATK:" + std::to_string(ci.atk) +
                            " DEF:" + std::to_string(ci.def) +
                            " MP:" + std::to_string(ci.mana);
        std::string nameRole = std::string(ci.name) + "  [ " + ci.role + " ]";
        int cardW = std::max({ utf8Len(nameRole), utf8Len(stats), utf8Len(ci.desc), 20 }) + 4;
        int cardX = (scr.cols() - cardW) / 2;
        if (cardX < 1) cardX = 1;
        int cardH = 7;

        drawBorder(scr, cardX, cardY, cardW, cardH, COL_YELLOW);
        scr.putStr(cardX + 2, cardY + 1, nameRole, COL_YELLOW, COL_BLACK, CELL_BOLD);
        for (int x = 0; x < cardW - 2; x++)
            scr.put(cardX + 1 + x, cardY + 2, 0x2550, COL_YELLOW);
        scr.put(cardX, cardY + 2, 0x2560, COL_YELLOW);
        scr.put(cardX + cardW - 1, cardY + 2, 0x2563, COL_YELLOW);
        scr.putStr(cardX + 2, cardY + 3, stats, COL_GREEN);
        scr.putStr(cardX + 2, cardY + 4, ci.desc, COL_GRAY, COL_BLACK, CELL_DIM);
        scr.putStr(cardX + 2, cardY + 5, "~~ CLASE SECRETA ~~", COL_MAGENTA, COL_BLACK, CELL_DIM);

    } else {
        // ─── Modo clases normales: 3 filas (Física/Mágica/Distancia) + retrato ─
        static constexpr float kPortScale = 1.0f;
        static constexpr int kSrcSize = 60;
        int portW = kSrcSize;
        int portH = kSrcSize / 2;

        int listCol = 2;
        int boxW = 36;
        for (int k = 0; k < 3; k++) {
            const auto& c = kClasses[k];
            std::string st = "HP:" + std::to_string(c.hp) + " ATK:" + std::to_string(c.atk) +
                             " DEF:" + std::to_string(c.def) + " MP:" + std::to_string(c.mana);
            boxW = std::max({ boxW, utf8Len(c.desc) + 4, utf8Len(st) + 4,
                              utf8Len(c.name) + 8 });
        }
        int boxH = 7;
        int startRow = 4;

        for (int cat = 0; cat < 3; cat++) {
            const auto& ci = kClasses[cat];
            bool sel = (cat == category);
            Color tc = sel ? COL_YELLOW : COL_GRAY;
            uint8_t tf = sel ? CELL_BOLD : CELL_DIM;
            int br = startRow + cat * (boxH + 1);

            drawBorder(scr, listCol, br, boxW, boxH, tc);

            // Categoría en la esquina
            scr.putStr(listCol + 2, br + 1,
                       std::string("[ ") + kCategoryNames[cat] + " ]", tc, COL_BLACK, CELL_DIM);

            scr.putStr(listCol + 2, br + 2, ci.name, tc, COL_BLACK, tf);

            // Separador
            for (int x = 0; x < boxW - 2; x++)
                scr.put(listCol + 1 + x, br + 3, 0x2550, tc);
            scr.put(listCol, br + 3, 0x2560, tc);
            scr.put(listCol + boxW - 1, br + 3, 0x2563, tc);

            // Stats
            std::string manaLabel = (cat == 0) ? " AG:" : " MP:";
            scr.putStr(listCol + 2, br + 4,
                "HP:" + std::to_string(ci.hp) +
                " ATK:" + std::to_string(ci.atk) +
                " DEF:" + std::to_string(ci.def) +
                manaLabel + std::to_string(ci.mana),
                sel ? COL_GREEN : COL_DK_GREEN, COL_BLACK, tf);

            // Descripción
            scr.putStr(listCol + 2, br + 5, ci.desc, tc, COL_BLACK, CELL_DIM);
        }

        int spaceStart = listCol + boxW + 2;
        int portCol = spaceStart + (scr.cols() - spaceStart - portW) / 2;
        if (portCol < spaceStart) portCol = spaceStart;
        drawBorder(scr, portCol - 1, 3, portW + 2, portH + 2, COL_YELLOW);
        drawPortrait(scr, portCol, 4, kClasses[category].portrait, kPortScale);
    }

    drawCentered(scr, scr.rows() - 2, 0, scr.cols(),
                 secretUnlocked
                     ? "A/D ◄► navegar  |  ENTER confirmar  |  ESC volver"
                     : "W/S ▲▼ clase  |  ENTER confirmar  |  ESC volver",
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
        for (int x = 0; x < pw - 2; x++)
            scr.put(px + 1 + x, py + 5, 0x2550, bc);
        scr.put(px, py + 5, 0x2560, bc);
        scr.put(px + pw - 1, py + 5, 0x2563, bc);
        // barra inferior simulada
        scr.putStr(px + 1, py + 6, "Hero Lv1 HP\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91 MP\xe2\x96\x91\xe2\x96\x91",
                   COL_WHITE, COL_BLACK, CELL_DIM);
        // etiqueta
        std::string label = "Barra inferior";
        scr.putStr(px + (pw - (int)label.size()) / 2, py + ph,
                   label, bc, COL_BLACK, sel ? CELL_BOLD : CELL_DIM);
    }

    drawCentered(scr, cy + 4, 0, scr.cols(),
                 "A/D ◄► navegar  |  ENTER confirmar  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

// ─── drawSettings ──────────────────────────────────────────────────────────────

void Renderer::drawSettings(TerminalScreen& scr, int selection, HudLayout hud,
                            int mapZoom, bool shaderOn, bool arrowMovement, int sfxVolume) {
    int cy = scr.rows() / 2;
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
    const char* moveVal = arrowMovement ? "Flechas" : "WASD";
    std::string sfxStr  = "◄ " + std::string(sfxVolume / 10, '|') + std::string(10 - sfxVolume / 10, '.')
                        + (sfxVolume == 0 ? " Off" : " " + std::to_string(sfxVolume) + "%") + " ►";

    Setting items[] = {
        {"Estilo HUD",   hudVal},
        {"Zoom Mapa",    zoomVal},
        {"Shader CRT",   shaderVal},
        {"Movimiento",   moveVal},
        {"Efectos",      sfxStr.c_str()},
        {"Volver",       ""},
    };

    int labelW = 0;
    for (const auto& it : items) labelW = std::max(labelW, utf8Len(it.label));
    std::vector<BoxLine> lines;
    for (int i = 0; i < (int)std::size(items); i++) {
        bool sel = (i == selection);
        std::string line = std::string(items[i].label);
        if (items[i].value[0] != '\0')
            line += std::string(labelW - utf8Len(items[i].label) + 4, ' ') +
                    "[" + items[i].value + "]";
        lines.push_back({ line, sel ? COL_YELLOW : COL_WHITE,
                          static_cast<uint8_t>(sel ? (CELL_BOLD | CELL_INVERTED) : 0) });
    }
    drawFitBox(scr, cy - 2, lines, COL_GRAY, 34);

    drawCentered(scr, cy + 7, 0, scr.cols(),
                 "W/S ▲▼ navegar  |  ENTER cambiar  |  A/D ◄► volumen  |  ESC volver",
                 COL_GRAY, CELL_DIM);
}

void Renderer::drawExploration(TerminalScreen& scr, const Map& map,
                                const Player& player, HudLayout layout,
                                const std::vector<MapEntity>& entities,
                                const std::vector<Position>& torches,
                                const std::string& message, int mapZoom,
                                int scrollTick, bool isVillage, const DayLight& light,
                                bool arrowMovement) {
    // Hora del pueblo sobre el borde superior del mapa: " ● 10:30 Día "
    auto drawClock = [&]() {
        if (!isVillage) return;
        int hh = static_cast<int>(light.hour);
        int mm = static_cast<int>((light.hour - hh) * 60.0f) / 10 * 10;
        char time[16];
        std::snprintf(time, sizeof(time), "%02d:%02d", hh, mm);
        int icon; Color ic;
        switch (light.phase) {
        case DayPhase::Day:   icon = 0x25CF; ic = COL_YELLOW;            break;
        case DayPhase::Night: icon = 0x25D0; ic = {170, 190, 255, 255};  break;
        default:              icon = 0x25D1; ic = COL_ORANGE;            break;
        }
        int x = 2;
        scr.put(x++, 0, ' ', COL_WHITE);
        scr.put(x++, 0, icon, ic, COL_BLACK, CELL_BOLD);
        x += scr.putStr(x, 0, std::string(" ") + time + " ", COL_WHITE);
        scr.putStr(x, 0, std::string(dayPhaseName(light.phase)) + " ", ic);
    };

    if (layout == HudLayout::Sidebar) {
        int panelW = 30;
        int mapW   = scr.cols() - panelW - 1;
        drawMap(scr, 1, 1, mapW - 2, scr.rows() - 2, map, entities, torches,
                colorForPlayerClass(player.getClass()), player.getDungeonFloor(), isVillage, light);
        drawBorder(scr, 0, 0, mapW, scr.rows());
        drawClock();
        drawHudPanel(scr, mapW + 1, 0, player, mapZoom, scrollTick, arrowMovement);
        if (!message.empty())
            drawCentered(scr, scr.rows() / 2, 1, mapW - 2,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED | CELL_ITALIC);
    } else {
        int hudH = 13;
        int mapH = scr.rows() - hudH;
        drawMap(scr, 1, 1, scr.cols() - 2, mapH - 2, map, entities, torches,
                colorForPlayerClass(player.getClass()), player.getDungeonFloor(), isVillage, light);
        drawBorder(scr, 0, 0, scr.cols(), mapH);
        drawClock();
        drawHudBar(scr, mapH, player, mapZoom, arrowMovement);
        if (!message.empty())
            drawCentered(scr, mapH / 2, 1, scr.cols() - 2,
                         " " + message + " ", COL_YELLOW, CELL_BOLD | CELL_INVERTED | CELL_ITALIC);
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

    // Log (derecha)
    drawVSep(scr, halfW, 0, rows - 8);
    int lr = 1;
    scr.putStr(halfW + 2, lr++, "LOG", COL_WHITE, COL_BLACK, CELL_BOLD);
    drawHSep(scr, halfW + 1, lr++, halfW - 1);
    const auto& log = combat.getLog();
    int logStart = std::max(0, static_cast<int>(log.size()) - (rows - 12));
    for (int i = logStart; i < static_cast<int>(log.size()) && lr < br - 1; i++)
        scr.putStr(halfW + 2, lr++, log[i], logColor(log[i]));

    drawHSep(scr, 0, br++, cols);
    // Cada campo empieza tras el anterior: un nombre largo no pisa los valores
    int px = 1;
    px += scr.putStr(px, br, player.getName() + " [" + className(player) + "]",
                     COL_CYAN, COL_BLACK, CELL_BOLD) + 3;
    Color phpc = hpColor(player.getHp(), player.getMaxHp());
    px += scr.putStr(px, br, "HP: ", phpc, COL_BLACK, CELL_BOLD);
    px += scr.putStr(px, br, std::to_string(player.getHp()) + "/" +
                     std::to_string(player.getMaxHp()), phpc) + 3;
    std::string mpLbl2 = player.getClass() == PlayerClass::Warrior ? "AG: " : "MP: ";
    px += scr.putStr(px, br, mpLbl2, COL_CYAN, COL_BLACK, CELL_BOLD);
    scr.putStr(px, br++, std::to_string(player.getMana()) + "/" +
               std::to_string(player.getMaxMana()), COL_CYAN);
    if (!ptags.empty()) apStr += "  " + ptags;
    scr.putStr(1, br++, apStr, COL_YELLOW);
    drawHSep(scr, 0, br++, cols);

    if (!showingArts) {
        std::string mpCost = player.getClass() == PlayerClass::Warrior ? "AG" : "MP";
        int lootPct = combat.getLootChance();
        struct Action { std::string label; Color c; uint8_t fl; };
        const Action actions[9] = {
            { "[1] Atacar  1PA", ap >= 1 ? COL_WHITE : COL_GRAY, 0 },
            { "[2] At.Fuerte  2PA 8" + mpCost,
              ap >= 2 && player.getMana() >= 8 ? COL_WHITE : COL_GRAY, 0 },
            { "[3] Habilidades", COL_WHITE, 0 },
            { "[4] Defender  1PA", ap >= 1 ? COL_WHITE : COL_GRAY, 0 },
            { "[5] Poción (" + std::to_string(player.countHpPotions()) + ")  1PA",
              player.countHpPotions() > 0 && ap >= 1 ? COL_CYAN : COL_GRAY, 0 },
            { "[6] Huir  3PA", ap >= 3 ? COL_WHITE : COL_GRAY, 0 },
            { "[7] Saquear (" + std::to_string(lootPct) + "%)  1PA",
              (ap >= 1 && lootPct > 0) ? COL_WHITE : COL_GRAY, 0 },
            { "[SPACE] Fin turno", COL_WHITE, 0 },
            { "[TAB] Cambiar objetivo", COL_GRAY, CELL_DIM },
        };
        // Columnas tan anchas como la acción más larga; si no caben tres, se
        // reparten en tercios de pantalla como antes
        int colW = 0;
        for (const auto& a : actions) colW = std::max(colW, utf8Len(a.label) + 3);
        if (1 + 3 * colW > cols) colW = cols / 3;
        for (int i = 0; i < 9; i++) {
            const auto& a = actions[i];
            scr.putStr(1 + (i % 3) * colW, br, fitText(a.label, colW - 1), a.c, COL_BLACK, a.fl);
            if (i % 3 == 2) br++;
        }
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
            std::string line = "[" + std::to_string(i + 1) + "] " + art.label() +
                "  " + std::to_string(art.apCost) + "PA  " +
                std::to_string(art.manaCost) + mpUnit + "  " + art.description;
            Color rc = canAfford ? COL_WHITE : COL_GRAY;
            uint8_t rf = sel ? CELL_INVERTED : (canAfford ? 0 : CELL_DIM);
            if (sel && canAfford) rc = COL_YELLOW;
            scr.putStr(1, br++, line, rc, COL_BLACK, rf);
        }
        scr.putStr(1, br, "W/S ▲▼  |  ENTER confirmar",
                   COL_GRAY, COL_BLACK, CELL_DIM);
    }
}

// ─── drawInventory ────────────────────────────────────────────────────────────

void Renderer::drawInventory(TerminalScreen& scr, const Player& player, int selection) {
    const auto& items = player.getInventory().items();
    auto itemLine = [&](const Item& item, Color& ic) {
        std::string tag; ic = COL_WHITE;
        if (item.type == ItemType::Weapon) {
            tag = "[ARMA]   "; ic = colorForTier(item.tier);
        } else if (item.type == ItemType::Armor) {
            tag = "[ARMADURA]"; ic = colorForTier(item.tier);
        } else if (item.type == ItemType::Consumable && item.statBonus > 0) {
            tag = "[POCIÓN] "; ic = COL_CYAN;
        } else if (item.type == ItemType::Consumable && item.name == "Pala") {
            tag = "[PALA]   "; ic = COL_BROWN;
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
        return line;
    };
    const std::string statsLine =
        "ATK:" + std::to_string(player.getAttack()) +
        "  DEF:" + std::to_string(player.getDefense()) +
        "  $" + std::to_string(player.getCoins()) +
        "  Slots:" + std::to_string(player.getInventory().usedSlots()) +
        "/" + std::to_string(player.getInventory().totalSlots());
    const std::string helpLine = "[W/S ▲▼] Navegar  [E] Equipar  [U] Usar  [ESC] Cerrar";

    // Ancho según la línea más larga; mínimo 60 para que la ventana no salte
    // de tamaño con cada objeto que entra o sale
    int contentW = std::max(utf8Len(statsLine), utf8Len(helpLine));
    for (const auto& item : items) {
        Color unused;
        contentW = std::max(contentW, utf8Len(itemLine(item, unused)));
    }
    int cx = scr.cols() / 2;
    int w  = std::clamp(contentW + 4, std::min(60, scr.cols()), scr.cols());
    int sc2 = cx - w / 2, r = 2;

    // Retrato del jugador a la izquierda del panel (si hay espacio)
    if (sc2 >= 26) {
        int px = 1, py = 2;
        int bw = sc2 - 2;
        const char* miniPf = portraitForClass(player.getClass());
        static constexpr int kMiniH = 10;
        int bh = kMiniH + 1 + 7 + 1 + 2 + 2;

        drawBorder(scr, px, py, bw, bh);
        if (miniPf) {
            int pox = px + 1 + (bw - 2 - 20) / 2;
            drawPortrait(scr, pox, py + 1, miniPf, 1.0f);
        }

        int ly = py + kMiniH + 1;
        for (int x = 0; x < bw - 2; x++)
            scr.put(px + 1 + x, ly, 0x2550, COL_WHITE);
        scr.put(px, ly, 0x2560, COL_WHITE);
        scr.put(px + bw - 1, ly, 0x2563, COL_WHITE);

        int barLen = std::max(2, std::min(20, bw - 14));
        int barCol = px + 5;
        int indent = px + 2;
        ly++;
        scr.putStr(indent, ly, player.getName(),
                   colorForPlayerClass(player.getClass()), COL_BLACK, CELL_BOLD);

        ly++;
        scr.putStr(indent, ly,
                   "Nv." + std::to_string(player.getLevel())
                   + "  " + className(player), COL_WHITE);

        ly++;
        scr.putStr(indent, ly, "HP ", COL_GREEN);
        int hpPct = player.getMaxHp() > 0
            ? (player.getHp() * barLen / player.getMaxHp()) : 0;
        for (int i = 0; i < barLen; i++)
            scr.put(barCol + i, ly,
                    i < hpPct ? 0x2588 : 0x2591, COL_GREEN, COL_BLACK, 0);
        scr.putStr(barCol + barLen + 1, ly,
                   " " + std::to_string(player.getHp())
                   + "/" + std::to_string(player.getMaxHp()),
                   COL_GREEN, COL_BLACK, CELL_DIM);

        ly++;
        std::string mpLbl = player.getClass() == PlayerClass::Warrior ? "AG " : "MP ";
        int mpPct = player.getMaxMana() > 0
            ? (player.getMana() * barLen / player.getMaxMana()) : 0;
        scr.putStr(indent, ly, mpLbl, COL_CYAN);
        for (int i = 0; i < barLen; i++)
            scr.put(barCol + i, ly,
                    i < mpPct ? 0x2588 : 0x2591, COL_CYAN, COL_BLACK, 0);
        scr.putStr(barCol + barLen + 1, ly,
                   " " + std::to_string(player.getMana())
                   + "/" + std::to_string(player.getMaxMana()),
                   COL_CYAN, COL_BLACK, CELL_DIM);

        ly++;
        scr.putStr(indent, ly,
                   "ATK " + std::to_string(player.getAttack())
                   + "  DEF " + std::to_string(player.getDefense()),
                   COL_GRAY);

        ly++;
        scr.putStr(indent, ly,
                   "$ " + std::to_string(player.getCoins()),
                   COL_YELLOW);

        ly++;
        scr.putStr(indent, ly,
                   "Piso " + std::to_string(player.getDungeonFloor()),
                   COL_GRAY, COL_BLACK, CELL_DIM);

        ly++;
        {
            int sw = bw - 2;
            std::string title = " EQUIPO ";
            int titleStart = px + 1 + (sw - (int)title.size()) / 2;
            for (int x = 0; x < sw; x++)
                scr.put(px + 1 + x, ly, 0x2550, COL_WHITE);
            scr.put(px, ly, 0x2560, COL_WHITE);
            scr.put(px + bw - 1, ly, 0x2563, COL_WHITE);
            scr.putStr(titleStart, ly, title, COL_WHITE, COL_BLACK, CELL_BOLD);
        }
        ly++;
        auto equipLine = [&](const std::string& label,
                             const std::optional<Item>& slot) {
            if (slot)
                scr.putStr(indent, ly, label + slot->name
                           + " +" + std::to_string(slot->statBonus), colorForTier(slot->tier));
            else
                scr.putStr(indent, ly, label + "(vacio)", COL_GRAY, COL_BLACK, CELL_DIM);
            ly++;
        };
        equipLine("Arma:  ", player.getEquippedWeapon());
        equipLine("Armad: ", player.getEquippedArmor());
    }

    drawBorder(scr, sc2, 1, w, scr.rows() - 2);

    scr.putStr(sc2 + 2, r++, "=== MOCHILA ===", COL_CYAN, COL_BLACK, CELL_BOLD);
    if (items.empty()) {
        scr.putStr(sc2 + 2, r++, "(vacia)", COL_GRAY, COL_BLACK, CELL_DIM);
    } else {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            Color ic;
            std::string line = itemLine(items[i], ic);
            uint8_t f = (selection == i) ? CELL_INVERTED : 0;
            scr.putStr(sc2 + 2, r++, line, ic, COL_BLACK, f);
        }
    }
    for (int x = 0; x < w - 2; x++)
        scr.put(sc2 + 1 + x, r, 0x2550, COL_WHITE);
    scr.put(sc2, r, 0x2560, COL_WHITE);
    scr.put(sc2 + w - 1, r, 0x2563, COL_WHITE);
    r++;
    scr.putStr(sc2 + 2, r++, statsLine, COL_GREEN);
    scr.putStr(sc2 + 2, r, helpLine, COL_GRAY, COL_BLACK, CELL_DIM);
}

// ─── drawQuestLog ─────────────────────────────────────────────────────────────

void Renderer::drawQuestLog(TerminalScreen& scr,
                             const std::vector<Quest>& quests, int selection) {
    auto statusInfo = [](QuestStatus st, std::string& icon, std::string& label, Color& c) {
        switch (st) {
            case QuestStatus::Completed:  c = COL_GREEN;  icon = "[+] "; label = "COMPLETADA";  break;
            case QuestStatus::InProgress: c = COL_YELLOW; icon = "[~] "; label = "EN PROGRESO"; break;
            case QuestStatus::Failed:     c = COL_RED;    icon = "[x] "; label = "FALLIDA";     break;
            default:                      c = COL_GRAY;   icon = "[ ] "; label = "PENDIENTE";   break;
        }
    };
    auto rewardLine = [](const Quest& q) {
        return "XP: " + std::to_string(q.xpReward) + "   Oro: " + std::to_string(q.goldReward);
    };

    // Anchos según el contenido de todas las misiones: la ventana no cambia
    // de tamaño al moverse por la lista. La descripción se ajusta por palabras.
    const std::string footer = " W/S ▲▼ navegar  |  ESC volver ";
    int listContent = 0, detailContent = 30;
    for (const auto& q : quests) {
        std::string icon, label; Color c;
        statusInfo(q.status, icon, label, c);
        listContent   = std::max(listContent, utf8Len(icon + q.title));
        detailContent = std::max({ detailContent, utf8Len(q.title), utf8Len(rewardLine(q)) });
        for (const auto& obj : q.objectives)
            detailContent = std::max(detailContent, utf8Len(obj.description) + 2);
    }
    detailContent = std::min(detailContent, 50);

    int listW   = std::max(listContent + 2, 20);
    int detailW = detailContent + 3;
    int w = std::max(listW + detailW + 1, utf8Len(footer) + 2);
    // Sin espacio: primero se estrecha el detalle y después la lista
    int maxW = scr.cols() - 4;
    if (w > maxW) {
        int excess = w - maxW;
        int cut = std::min(excess, std::max(0, detailW - 23));
        detailW -= cut; excess -= cut;
        listW = std::max(12, listW - excess);
        w = std::min(maxW, listW + detailW + 1);
    }

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

    int detailX = x0 + listW + 1;
    int textW   = detailW - 3;
    drawVSep(scr, x0 + listW, y0 + 1, h - 2);

    int listY = y0 + 1;
    for (int i = 0; i < static_cast<int>(quests.size()); i++) {
        const auto& q = quests[i];
        std::string icon, label; Color c;
        statusInfo(q.status, icon, label, c);
        std::string line = fitText(icon + q.title, listW - 2);
        uint8_t f = 0;
        if (i == selection) {
            f = CELL_INVERTED;
            line += std::string(std::max(0, listW - 2 - utf8Len(line)), ' ');
        }
        scr.putStr(x0 + 1, listY + i, line, c, COL_BLACK, f);
    }

    if (selection >= 0 && selection < (int)quests.size()) {
        const auto& q = quests[selection];
        int dy = y0 + 1;
        scr.putStr(detailX + 1, dy++, fitText(q.title, textW), COL_YELLOW, COL_BLACK, CELL_BOLD);
        dy++;
        std::string icon, statusStr; Color sc;
        statusInfo(q.status, icon, statusStr, sc);
        scr.putStr(detailX + 1, dy++, statusStr, sc, COL_BLACK, CELL_BOLD);
        dy++;
        for (const auto& l : wrapText(q.description, textW)) {
            if (dy >= y0 + h - 4) break;
            scr.putStr(detailX + 1, dy++, l, COL_WHITE);
        }
        dy++;
        scr.putStr(detailX + 1, dy++, "Objetivos:", COL_CYAN, COL_BLACK, CELL_BOLD);
        for (const auto& obj : q.objectives) {
            if (dy >= y0 + h - 2) break;
            std::string objLine = (obj.completed ? "\xe2\x96\xa0 " : "\xe2\x96\xa1 ") + obj.description;
            scr.putStr(detailX + 1, dy++, fitText(objLine, textW),
                       obj.completed ? COL_GREEN : COL_GRAY);
        }
        if (dy < y0 + h - 2) {
            dy++;
            scr.putStr(detailX + 1, dy, fitText(rewardLine(q), textW), COL_YELLOW);
        }
    }

    drawCentered(scr, y0 + h - 1, x0, w, footer, COL_GRAY, CELL_DIM);
}

// ─── drawBestiary ──────────────────────────────────────────────────────────────

void Renderer::drawBestiary(TerminalScreen& scr,
                             const std::array<BestiaryEntry, kBestiaryEntryCount>& bestiary,
                             int selection) {
    int cols = scr.cols();
    int rows = scr.rows();

    // Anchos a partir de los datos de todas las criaturas (descubiertas o no),
    // para que la ventana no cambie al descubrir una nueva.
    int nameW = 3, killsW = 2;
    for (int i = 0; i < kBestiaryEntryCount; i++) {
        nameW  = std::max(nameW, utf8Len(kBestiaryData[i].name));
        killsW = std::max(killsW, static_cast<int>(std::to_string(bestiary[i].kills).size()));
    }
    // glifo + espacio + nombre + " x" + kills
    int listW = std::max(20, 2 + nameW + 2 + killsW + 2);

    int detailContent = std::max({ 26, nameW + 2,
                                   utf8Len("Kills: 9999  Encontrado piso 20") });
    for (const auto& info : kBestiaryData)
        detailContent = std::max(detailContent, std::min(utf8Len(info.special), 44));
    int detailW = detailContent + 3;

    const std::string footer = " W/S ▲▼ navegar  |  ESC volver ";
    int w = std::min(cols - 4, std::max(listW + detailW + 1, utf8Len(footer) + 2));
    detailW = std::max(10, w - listW - 1);
    int textW = detailW - 3;

    int x0 = (cols - w) / 2;
    int y0 = 1;
    int h = rows - 2;

    drawBorder(scr, x0, y0, w, h);
    drawCentered(scr, y0, x0, w, " BESTIARIO ", COL_CYAN, CELL_BOLD);

    int detailX = x0 + listW + 1;
    drawVSep(scr, x0 + listW, y0 + 1, h - 2);

    int ly = y0 + 1;
    for (int i = 0; i < kBestiaryEntryCount; i++) {
        const auto& be = bestiary[i];
        uint8_t f = (i == selection) ? CELL_INVERTED : 0;
        if (be.discovered) {
            Color c = colorFromPair(kBestiaryData[i].colorPair);
            std::string kills = " x" + std::to_string(be.kills);
            int nameSpace = listW - 2 - 2 - utf8Len(kills);
            std::string name = fitText(be.name, nameSpace);
            name += std::string(std::max(0, nameSpace - utf8Len(name)), ' ');
            scr.put(x0 + 1, ly, kBestiaryData[i].glyph, c, COL_BLACK, f);
            scr.put(x0 + 2, ly, ' ', c, COL_BLACK, f);
            scr.putStr(x0 + 3, ly, name, c, COL_BLACK, f);
            scr.putStr(x0 + 3 + nameSpace, ly, kills, COL_WHITE, COL_BLACK, f);
        } else {
            std::string line = "  ???";
            line += std::string(std::max(0, listW - 2 - utf8Len(line)), ' ');
            scr.putStr(x0 + 1, ly, line, COL_GRAY, COL_BLACK, f | CELL_DIM);
        }
        ly++;
    }

    if (selection >= 0 && selection < kBestiaryEntryCount) {
        const auto& be = bestiary[selection];
        const auto& info = kBestiaryData[selection];
        int dx = detailX + 1;
        int dy = y0 + 1;

        if (be.discovered) {
            Color gc = colorFromPair(info.colorPair);
            scr.put(dx, dy, info.glyph, gc);
            scr.putStr(dx + 2, dy, fitText(be.name, textW - 2), gc, COL_BLACK, CELL_BOLD);
            dy++;

            scr.putStr(dx, dy++, fitText("Kills: " + std::to_string(be.kills)
                       + "  Encontrado piso " + std::to_string(be.firstSeenFloor), textW),
                       COL_WHITE);

            dy++;
            scr.putStr(dx, dy++, "Stats base:", COL_GRAY, COL_BLACK, CELL_BOLD);
            scr.putStr(dx, dy++, "HP  " + std::to_string(info.baseHp), COL_WHITE);
            scr.putStr(dx, dy++, "ATK " + std::to_string(info.baseAtk), COL_WHITE);
            scr.putStr(dx, dy++, "DEF " + std::to_string(info.baseDef), COL_WHITE);
            scr.putStr(dx, dy++, "PA  " + std::to_string(info.basePa), COL_WHITE);
            scr.putStr(dx, dy++, "Piso " + std::to_string(info.firstFloor) + "+", COL_WHITE);

            if (!info.special.empty()) {
                dy++;
                for (const auto& l : wrapText(info.special, textW)) {
                    if (dy >= y0 + h - 1) break;
                    scr.putStr(dx, dy++, l, COL_YELLOW, COL_BLACK, CELL_DIM);
                }
            }
        } else {
            scr.putStr(dx, dy++, "???", COL_GRAY, COL_BLACK, CELL_DIM);
            dy++;
            for (const auto& l : wrapText("Aun no has encontrado esta criatura.", textW))
                scr.putStr(dx, dy++, l, COL_GRAY, COL_BLACK, CELL_DIM);
        }
    }

    drawCentered(scr, y0 + h - 1, x0, w, footer, COL_GRAY, CELL_DIM);
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
    const char* opts[] = { "Menú Principal", "Salir al escritorio" };
    std::vector<BoxLine> lines = {
        { "Salir del juego",    COL_WHITE, CELL_BOLD, false, true },
        { "¿Qué deseas hacer?", COL_GRAY,  CELL_DIM,  false, true },
        {},
    };
    for (int i = 0; i < 2; i++) {
        bool sel = (i == selection);
        lines.push_back({ std::string("  ") + opts[i] + "  ",
                          sel ? COL_YELLOW : COL_WHITE,
                          static_cast<uint8_t>(sel ? (CELL_BOLD | CELL_INVERTED) : 0),
                          false, true });
    }
    lines.push_back({ "ESC para cancelar", COL_GRAY, CELL_DIM, false, true });
    int h = static_cast<int>(lines.size()) + 2;
    drawFitBox(scr, scr.rows() / 2 - h / 2, lines, COL_YELLOW, 30);
}

// ─── Ventanas auto-ajustables ─────────────────────────────────────────────────

static std::string padRight(const std::string& s, int width) {
    int len = utf8Len(s);
    return len >= width ? s : s + std::string(width - len, ' ');
}

static std::string padLeft(const std::string& s, int width) {
    int len = utf8Len(s);
    return len >= width ? s : std::string(width - len, ' ') + s;
}

void Renderer::drawFitBox(TerminalScreen& scr, int row,
                          const std::vector<BoxLine>& lines, Color border, int minW) {
    int content = 0;
    for (const auto& l : lines)
        if (!l.sep) content = std::max(content, utf8Len(l.text));
    int w  = std::clamp(content + 4, std::min(minW, scr.cols()), scr.cols());
    int bx = (scr.cols() - w) / 2;
    int h  = static_cast<int>(lines.size()) + 2;
    int inner = w - 4;
    drawBorder(scr, bx, row, w, h, border);

    for (int i = 0; i < static_cast<int>(lines.size()); i++) {
        const BoxLine& l = lines[i];
        int r = row + 1 + i;
        if (l.sep) {
            for (int x = 1; x < w - 1; x++)
                scr.put(bx + x, r, 0x2550, border);
            scr.put(bx, r, 0x2560, border);
            scr.put(bx + w - 1, r, 0x2563, border);
            continue;
        }
        std::string text = fitText(l.text, inner);
        if (l.center) {
            drawCentered(scr, r, bx, w, text, l.fg, l.flags);
        } else {
            // La selección invertida ocupa todo el ancho para que se lea como fila
            if (l.flags & CELL_INVERTED) text = padRight(text, inner);
            scr.putStr(bx + 2, r, text, l.fg, COL_BLACK, l.flags);
        }
    }
}

// ─── drawShop ─────────────────────────────────────────────────────────────────

void Renderer::drawShop(TerminalScreen& scr, const std::vector<ShopItem>& stock,
                         int selection, const Player& player,
                         const std::string& message,
                         bool sellMode, int sellSelection, int quantity) {
    std::vector<BoxLine> lines;
    lines.push_back({ sellMode ? "=== VENDER ===" : "=== TIENDA DEL PUEBLO ===",
                      COL_ORANGE, CELL_BOLD });
    lines.push_back({ "$ " + std::to_string(player.getCoins()) + " monedas disponibles",
                      COL_ORANGE });
    lines.push_back(BoxLine::separator());

    if (sellMode) {
        const auto& inv = player.getInventory().items();
        auto sellable = [](const Item& it) {
            return it.type == ItemType::Weapon || it.type == ItemType::Armor;
        };
        int nameW = 0, priceW = 0;
        for (const auto& it : inv) {
            if (!sellable(it)) continue;
            nameW  = std::max(nameW, utf8Len(it.name));
            priceW = std::max(priceW, static_cast<int>(std::to_string(it.value / 2).size()));
        }
        bool any = false;
        for (int i = 0; i < static_cast<int>(inv.size()); i++) {
            const auto& item = inv[i];
            if (!sellable(item)) continue;
            any = true;
            bool sel = (i == sellSelection);
            std::string tag = (item.type == ItemType::Weapon) ? "[ARMA]    " : "[ARMADURA]";
            lines.push_back({ tag + " " + padRight(item.name, nameW) + "   " +
                              padLeft(std::to_string(item.value / 2), priceW) + " $",
                              sel ? COL_ORANGE : colorForTier(item.tier),
                              static_cast<uint8_t>(sel ? CELL_INVERTED : 0) });
        }
        if (!any)
            lines.push_back({ "(nada que vender)", COL_GRAY, CELL_DIM });
    } else {
        int nameW = 0, priceW = 0, totalW = 0;
        for (const auto& s : stock) {
            nameW  = std::max(nameW, utf8Len(s.item.name));
            priceW = std::max(priceW, static_cast<int>(std::to_string(s.price).size()));
            totalW = std::max(totalW, static_cast<int>(std::to_string(s.price * s.item.quantity).size()));
        }
        // Columna de cantidad con ancho fijo para que la descripción no baile
        auto qtyColumn = [&](const ShopItem& s, bool sel) {
            if (s.sold || s.item.quantity <= 1) return std::string();
            std::string col = "  quedan " + padLeft(std::to_string(s.item.quantity), 2);
            if (sel)
                return col + "   ◄ " + padLeft(std::to_string(quantity), 2) + " ► = " +
                       padLeft(std::to_string(s.price * quantity), totalW) + " $";
            return col;
        };
        int qtyW = 0;
        for (int i = 0; i < static_cast<int>(stock.size()); i++)
            qtyW = std::max(qtyW, utf8Len(qtyColumn(stock[i], true)));
        for (int i = 0; i < static_cast<int>(stock.size()); i++) {
            const auto& s = stock[i];
            bool sel = (i == selection);
            std::string line = (s.sold ? "[VENDIDO] " : "          ") +
                               padRight(s.item.name, nameW) + "   " +
                               padLeft(std::to_string(s.price), priceW) + " $" +
                               padRight(qtyColumn(s, sel), qtyW);
            if (!s.item.description.empty())
                line += "   " + s.item.description;
            lines.push_back({ line,
                              s.sold ? COL_GRAY : (sel ? COL_ORANGE : shopItemColor(s.item)),
                              static_cast<uint8_t>(s.sold ? CELL_DIM : (sel ? CELL_INVERTED : 0)) });
        }
    }

    lines.push_back(BoxLine::separator());
    // Línea de mensaje siempre reservada: la ventana no cambia de alto al comprar
    lines.push_back({ message, COL_GREEN, CELL_BOLD });
    lines.push_back({ sellMode ? "W/S ▲▼ navegar  |  ENTER vender  |  V comprar  |  ESC salir"
                               : "W/S ▲▼ navegar  |  A/D ◄► cantidad  |  ENTER comprar  |  V vender  |  ESC salir",
                      COL_GRAY, CELL_DIM });
    drawFitBox(scr, 1, lines, COL_ORANGE);
}

// ─── drawSkills ───────────────────────────────────────────────────────────────

void Renderer::drawSkills(TerminalScreen& scr, const Player& player, int selection) {
    const auto  known    = player.getKnownArts();
    const auto& equipped = player.getEquippedArts();
    const char* res      = player.getClass() == PlayerClass::Warrior ? "AG" : "MP";

    int nameW = 0;
    for (const auto& a : known) nameW = std::max(nameW, utf8Len(a.label()));
    auto cost = [&](const Art& a) {
        return std::to_string(a.apCost) + " PA  " + padLeft(std::to_string(a.manaCost), 2) + " " + res;
    };
    auto slotOf = [&](ArtEffect e) {
        for (int i = 0; i < static_cast<int>(equipped.size()); i++)
            if (equipped[i] == e) return i;
        return -1;
    };

    std::vector<BoxLine> lines;
    lines.push_back({ "=== HABILIDADES ===", COL_CYAN, CELL_BOLD, false, true });
    lines.push_back(BoxLine::separator());
    lines.push_back({ "Equipadas (teclas 1-3 en combate):", COL_WHITE, CELL_BOLD });
    auto equippedArts = player.getAvailableArts();
    for (int i = 0; i < Player::kEquippedArtSlots; i++) {
        if (i < static_cast<int>(equippedArts.size()))
            lines.push_back({ "  [" + std::to_string(i + 1) + "] " + padRight(equippedArts[i].label(), nameW)
                              + "   " + cost(equippedArts[i]), COL_YELLOW });
        else
            lines.push_back({ "  [" + std::to_string(i + 1) + "] (vacía)", COL_GRAY, CELL_DIM });
    }
    lines.push_back(BoxLine::separator());
    lines.push_back({ "Conocidas:", COL_WHITE, CELL_BOLD });
    for (int i = 0; i < static_cast<int>(known.size()); i++) {
        const Art& a = known[i];
        int slot = slotOf(a.effect);
        bool sel = i == selection;
        std::string tag = slot >= 0 ? "[" + std::to_string(slot + 1) + "]" : "   ";
        lines.push_back({ tag + " " + padRight(a.label(), nameW) + "   " + cost(a) + "   " + a.description,
                          sel ? COL_ORANGE : (slot >= 0 ? COL_GREEN : COL_GRAY),
                          static_cast<uint8_t>(sel ? CELL_INVERTED : 0) });
    }
    lines.push_back(BoxLine::separator());
    lines.push_back({ "Las marcadas [Q]/[T] también se usan en el mapa si están equipadas.",
                      COL_GRAY, CELL_DIM });
    lines.push_back({ "W/S ▲▼ elegir  |  1/2/3 equipar en esa ranura  |  ESC volver",
                      COL_GRAY, CELL_DIM });
    drawFitBox(scr, 2, lines, COL_CYAN, 50);
}

void Renderer::drawVillageMenu(TerminalScreen& scr, VillageMenu menu, int selection,
                               const Player& player, int incursions, int trainingTab) {
    std::vector<BoxLine> lines;
    lines.push_back({ menu == VillageMenu::Forge ? "=== FORJA ===" : "=== ENTRENAMIENTO ===",
                      COL_ORANGE, CELL_BOLD });
    lines.push_back({ "$ " + std::to_string(player.getCoins()) + " monedas disponibles",
                      COL_ORANGE });
    lines.push_back(BoxLine::separator());

    // Filas "texto ... coste $" con la columna de precios alineada
    struct Row { std::string text; int cost; bool enabled; };
    std::vector<Row> rows;
    std::string help;

    if (menu == VillageMenu::Forge) {
        auto slotRow = [&](const char* tag, const std::optional<Item>& slot,
                           const char* empty, const char* stat) {
            int bonus = slot ? slot->statBonus : 0;
            std::string text = std::string(tag) + (slot
                ? slot->name + " +" + std::to_string(bonus) + " " + stat : empty);
            rows.push_back({ text, forgeCost(bonus, incursions), slot.has_value() });
        };
        slotRow("[ARMA]     ", player.getEquippedWeapon(), "(sin arma)", "ATK");
        slotRow("[ARMADURA] ", player.getEquippedArmor(), "(sin armadura)", "DEF");
        help = "ENTER forjar +1  |  ESC salir";
    } else if (trainingTab == 1) {
        lines.push_back({ "  Estadísticas  [ HABILIDADES ]", COL_WHITE, CELL_BOLD });
        lines.push_back({ "  II: +20% efecto (Nv 10)   III: +40% efecto y -25% maná (Nv 20)",
                          COL_GRAY, CELL_DIM });
        lines.push_back(BoxLine::separator());
        for (const auto& art : player.getKnownArts()) {
            if (art.rank >= kMaxArtRank) {
                rows.push_back({ art.name + "  " + artRankRoman(art.rank) + "  (máximo)", 0, false });
                continue;
            }
            int next = art.rank + 1;
            bool lvlOk = player.getLevel() >= artRankRequiredLevel(next);
            std::string text = art.name + "  " + artRankRoman(art.rank) + " -> " + artRankRoman(next);
            if (!lvlOk) text += "  (Nv " + std::to_string(artRankRequiredLevel(next)) + ")";
            rows.push_back({ text, artRankCost(next, incursions), lvlOk });
        }
        help = "ENTER mejorar  |  A/D pestaña  |  ESC salir";
    } else {
        lines.push_back({ "[ ESTADÍSTICAS ]  Habilidades", COL_WHITE, CELL_BOLD });
        lines.push_back(BoxLine::separator());
        auto stat = [](const char* label, int cur, int next) {
            return std::string(label) + "  (" + std::to_string(cur) + " -> " +
                   std::to_string(next) + ")";
        };
        rows.push_back({ stat("+10 Vida Max", player.getMaxHp(),   player.getMaxHp() + 10),
                         trainingCost(0, incursions), true });
        rows.push_back({ stat("+2  Ataque",   player.getAttack(),  player.getAttack() + 2),
                         trainingCost(1, incursions), true });
        rows.push_back({ stat("+1  Defensa",  player.getDefense(), player.getDefense() + 1),
                         trainingCost(2, incursions), true });
        rows.push_back({ stat("+5  Mana Max", player.getMaxMana(), player.getMaxMana() + 5),
                         trainingCost(3, incursions), true });
        help = "ENTER entrenar  |  A/D pestaña  |  ESC salir";
    }

    int textW = 0, costW = 0;
    for (const auto& r : rows) {
        textW = std::max(textW, utf8Len(r.text));
        costW = std::max(costW, static_cast<int>(std::to_string(r.cost).size()));
    }
    for (int i = 0; i < static_cast<int>(rows.size()); i++) {
        const Row& r = rows[i];
        bool sel = (i == selection);
        Color fg = sel ? COL_ORANGE : (r.enabled ? COL_WHITE : COL_GRAY);
        uint8_t fl = sel ? CELL_INVERTED : (r.enabled ? 0 : CELL_DIM);
        std::string price = r.cost > 0 ? padLeft(std::to_string(r.cost), costW) + " $"
                                       : std::string(costW + 2, ' ');
        lines.push_back({ padRight(r.text, textW) + "   " + price, fg, fl });
    }
    lines.push_back(BoxLine::separator());
    lines.push_back({ help, COL_GRAY, CELL_DIM });
    drawFitBox(scr, 1, lines, COL_ORANGE);
}

// ─── drawSkillTree ────────────────────────────────────────────────────────────

void Renderer::drawSkillTree(TerminalScreen& scr, const Player& player,
                             int /*tierCursor*/, int choiceCursor) {
    const SkillTreeDef& tree = getSkillTree(player.getClass());
    const auto& unlocked = player.getUnlockedSkills();

    // Título
    std::string title = std::string("ARBOL DE HABILIDADES - ") + className(player);
    auto choiceDesc = [](const SkillChoice& sc) {
        return sc.name + ": " + sc.desc +
               "  (PA:" + std::to_string(sc.apCost) +
               " MP:" + std::to_string(sc.manaCost) + ")";
    };
    // El ancho cubre todas las descripciones posibles: no cambia al mover el cursor
    int contentW = utf8Len(title);
    for (const auto& tier : tree.tiers)
        for (const auto& sc : tier.choices)
            contentW = std::max(contentW, utf8Len(choiceDesc(sc)));
    int boxW = std::max(52, contentW + 4);
    if (boxW > scr.cols() - 4) boxW = scr.cols() - 4;
    int boxX = (scr.cols() - boxW) / 2;
    if (boxX < 1) boxX = 1;
    int boxRow = 1;
    int innerW = boxW - 2;
    int innerX = boxX + 1;

    // Determinar tier pendiente (el que el jugador debe elegir)
    int pendingTier = -1;
    if (player.hasPendingSkillUnlock()) {
        for (int t = 0; t < static_cast<int>(tree.tiers.size()); t++) {
            bool already = false;
            for (const auto& us : unlocked)
                if (us.tier == t) { already = true; break; }
            if (!already) { pendingTier = t; break; }
        }
    }

    // Cabecera del cuadro
    scr.put(boxX, boxRow, 0x2554, COL_YELLOW);
    scr.put(boxX + boxW - 1, boxRow, 0x2557, COL_YELLOW);
    for (int x = 1; x < boxW - 1; x++)
        scr.put(boxX + x, boxRow, 0x2550, COL_YELLOW);
    boxRow++;

    drawCentered(scr, boxRow, innerX, innerW, title, COL_YELLOW, CELL_BOLD);
    scr.put(boxX, boxRow, 0x2551, COL_YELLOW);
    scr.put(boxX + boxW - 1, boxRow, 0x2551, COL_YELLOW);
    boxRow++;

    // Separador bajo el título
    scr.put(boxX, boxRow, 0x2560, COL_YELLOW);
    scr.put(boxX + boxW - 1, boxRow, 0x2563, COL_YELLOW);
    for (int x = 1; x < boxW - 1; x++)
        scr.put(boxX + x, boxRow, 0x2550, COL_YELLOW);
    boxRow++;

    int treeRow = boxRow;

    // Todo el árbol cuelga de una columna central; las ramas caen en
    // columnas fijas a cada lado para que esquinas y nodos queden alineados.
    const int midX      = innerX + innerW / 2;
    const int branchOff = std::max(7, std::min(12, innerW / 2 - 8));
    const int leftC     = midX - branchOff;
    const int rightC    = midX + branchOff;

    auto sideBorders = [&](int row) {
        scr.put(boxX, row, 0x2551, COL_YELLOW);
        scr.put(boxX + boxW - 1, row, 0x2551, COL_YELLOW);
    };
    auto hLine = [&](int x0, int x1, int row, Color c) {
        for (int x = x0; x <= x1; x++) scr.put(x, row, 0x2500, c);
    };
    // Centra el texto sobre una columna (no sobre un ancho)
    auto putAt = [&](int col, int row, const std::string& s, Color c, uint8_t fl) {
        scr.putStr(col - utf8Len(s) / 2, row, s, c, COL_BLACK, fl);
    };

    auto tierUnlockedChoice = [&](int t) {
        for (const auto& us : unlocked)
            if (us.tier == t) return us.choiceIdx;
        return -1;
    };
    auto trunkColor = [&](int t) {
        if (tierUnlockedChoice(t) >= 0) return COL_DK_GREEN;
        if (t == pendingTier)           return COL_YELLOW;
        return COL_DK_GRAY;
    };

    // Raíz del árbol
    putAt(midX, treeRow, "◆", COL_YELLOW, CELL_BOLD);
    sideBorders(treeRow);
    treeRow++;

    const int tierCount = static_cast<int>(tree.tiers.size());
    for (int t = 0; t < tierCount; t++) {
        const SkillTier& tier = tree.tiers[t];
        const int  unlockedChoice = tierUnlockedChoice(t);
        const bool isPending      = (t == pendingTier);
        const Color inC  = trunkColor(t);
        const Color outC = (t + 1 < tierCount) ? trunkColor(t + 1) : inC;

        // Tronco entrante
        scr.put(midX, treeRow, 0x2502, inC);
        sideBorders(treeRow);
        treeRow++;

        auto nodeStyle = [&](int ci, std::string& label, Color& c, uint8_t& fl) {
            const std::string& n = tier.choices[ci].name;
            if (unlockedChoice == ci)                 { label = "[" + n + "]"; c = COL_GREEN;   fl = CELL_BOLD; }
            else if (unlockedChoice >= 0)             { label = " " + n + " "; c = COL_DK_GRAY; fl = CELL_DIM;  }
            else if (isPending && choiceCursor == ci) { label = "►" + n + "◄"; c = COL_YELLOW; fl = CELL_BOLD; }
            else if (isPending)                       { label = "[" + n + "]"; c = COL_ORANGE;  fl = 0;         }
            else                                      { label = "[" + n + "]"; c = COL_DK_GRAY; fl = CELL_DIM;  }
        };
        auto branchColor = [&](int ci) {
            if (unlockedChoice >= 0) return unlockedChoice == ci ? COL_DK_GREEN : COL_DK_GRAY;
            if (isPending)           return choiceCursor == ci ? COL_YELLOW : COL_DK_GRAY;
            return COL_DK_GRAY;
        };

        Color lvlColor = (unlockedChoice >= 0) ? COL_GREEN : (isPending ? COL_YELLOW : COL_DK_GRAY);
        std::string lvlLabel = "Lv" + std::to_string(tier.requiredLevel);

        if (tier.choices.size() == 1) {
            std::string label; Color c; uint8_t fl;
            nodeStyle(0, label, c, fl);
            if (unlockedChoice < 0 && isPending) {
                label = "►" + tier.choices[0].name + "◄";
                c = COL_YELLOW; fl = CELL_BOLD;
            }
            putAt(midX, treeRow, label, c, fl);
            scr.putStr(innerX + 2, treeRow, lvlLabel, lvlColor);
            sideBorders(treeRow);
            treeRow++;
        } else {
            // ┌────┴────┐
            Color lc = branchColor(0), rc = branchColor(1);
            scr.put(leftC, treeRow, 0x250C, lc);
            hLine(leftC + 1, midX - 1, treeRow, lc);
            scr.put(midX, treeRow, 0x2534, inC);
            hLine(midX + 1, rightC - 1, treeRow, rc);
            scr.put(rightC, treeRow, 0x2510, rc);
            sideBorders(treeRow);
            treeRow++;

            for (int ci = 0; ci < 2; ci++) {
                std::string label; Color c; uint8_t fl;
                nodeStyle(ci, label, c, fl);
                putAt(ci == 0 ? leftC : rightC, treeRow, label, c, fl);
            }
            scr.putStr(innerX + 2, treeRow, lvlLabel, lvlColor);
            sideBorders(treeRow);
            treeRow++;

            // └────┬────┘
            scr.put(leftC, treeRow, 0x2514, lc);
            hLine(leftC + 1, midX - 1, treeRow, lc);
            scr.put(midX, treeRow, (t + 1 < tierCount) ? 0x252C : 0x2500, outC);
            hLine(midX + 1, rightC - 1, treeRow, rc);
            scr.put(rightC, treeRow, 0x2518, rc);
            sideBorders(treeRow);
            treeRow++;
        }
    }

    // Separador inferior antes de la descripción
    scr.put(boxX, treeRow, 0x2560, COL_YELLOW);
    scr.put(boxX + boxW - 1, treeRow, 0x2563, COL_YELLOW);
    for (int x = 1; x < boxW - 1; x++)
        scr.put(boxX + x, treeRow, 0x2550, COL_YELLOW);
    treeRow++;

    // Descripción de la habilidad destacada
    std::string descLine;
    if (pendingTier >= 0 && pendingTier < static_cast<int>(tree.tiers.size())) {
        const SkillTier& tier = tree.tiers[pendingTier];
        int ci2 = std::min(choiceCursor, static_cast<int>(tier.choices.size()) - 1);
        descLine = choiceDesc(tier.choices[ci2]);
    } else {
        descLine = "Habilidades de este nivel ya desbloqueadas.";
    }
    scr.put(boxX, treeRow, 0x2551, COL_YELLOW);
    scr.put(boxX + boxW - 1, treeRow, 0x2551, COL_YELLOW);
    descLine = fitText(descLine, innerW - 2);
    scr.putStr(innerX + 1, treeRow, descLine, COL_CYAN);
    treeRow++;

    // Pie del cuadro
    scr.put(boxX, treeRow, 0x255A, COL_YELLOW);
    scr.put(boxX + boxW - 1, treeRow, 0x255D, COL_YELLOW);
    for (int x = 1; x < boxW - 1; x++)
        scr.put(boxX + x, treeRow, 0x2550, COL_YELLOW);
    treeRow++;

    drawCentered(scr, treeRow, 0, scr.cols(),
                 pendingTier >= 0
                     ? "A/D ◄► elegir habilidad  |  ENTER confirmar"
                     : "ENTER o ESC para cerrar",
                 COL_GRAY, CELL_DIM);
}
