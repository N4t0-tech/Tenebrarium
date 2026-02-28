#include "Renderer.hpp"
#include "ui/XpLoader.hpp"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <stdexcept>

using namespace ftxui;

// ─── file-local helpers ──────────────────────────────────────────────────────

static std::vector<std::string> loadLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) lines.push_back(line);
    return lines;
}

static const char* className(const Player& p) {
    switch (p.getClass()) {
        case PlayerClass::Warrior: return "Guerrero";
        case PlayerClass::Mage:    return "Mago";
        case PlayerClass::Ranger:  return "Ranger";
    }
    return "";
}

static Color colorFromPair(int pair) {
    switch (pair) {
        case 2: return Color::Yellow;
        case 3: return Color::Cyan;
        case 4: return Color::Green;
        case 6: return Color::Red;
        case 7: return Color::GrayDark;
        default: return Color::White;
    }
}

// ─── makeStatBar ─────────────────────────────────────────────────────────────

Element Renderer::makeStatBar(int value, int max, Color col) {
    int w = 12;
    int fill = (max > 0) ? (value * w / max) : 0;
    fill = std::max(0, std::min(fill, w));
    Elements cells;
    for (int i = 0; i < w; i++)
        cells.push_back(text(i < fill ? "█" : "░") | color(col));
    return hbox(std::move(cells));
}

// ─── drawTitle ───────────────────────────────────────────────────────────────

Element Renderer::drawTitle(int selection) {
    auto titleLines = loadLines(ASSETS_DIR "/title.txt");
    Elements titleElems;
    for (const auto& l : titleLines)
        titleElems.push_back(text(l) | hcenter | color(Color::Cyan) | bold);

    auto opt = [&](const std::string& label, int idx) -> Element {
        if (selection == idx)
            return text("  " + label + "  ") | hcenter | color(Color::Yellow) | bold | inverted;
        return text("  " + label + "  ") | hcenter | color(Color::White);
    };

    return vbox({
        vbox(std::move(titleElems)),
        text("~ Un RPG de mazmorra y sombras ~") | hcenter | color(Color::White),
        text(""),
        opt("Nueva Partida", 0),
        opt("Salir", 1),
        text(""),
        text("Flechas para navegar  |  ENTER para confirmar") | hcenter | dim,
    }) | center | border;
}

// ─── drawNameInput ────────────────────────────────────────────────────────────

Element Renderer::drawNameInput(const std::string& name) {
    std::string display = name + "_";
    return vbox({
        text("T E N E B R A R I U M") | hcenter | color(Color::Cyan) | bold,
        text(""),
        text("Ingresa el nombre de tu personaje:") | hcenter | color(Color::White),
        text(""),
        hbox({text("> "), text(display), text("")}) | hcenter | color(Color::Yellow) | border,
        text(""),
        text("ENTER para continuar  |  ESC para volver") | hcenter | dim,
    }) | center | border;
}

// ─── drawClassSelect ─────────────────────────────────────────────────────────

struct ClassInfo {
    const char* name;
    const char* role;
    const char* desc;
    const char* portrait;   // nombre de archivo en assets/art/, nullptr si no hay
    int hp, atk, def, mana;
};
static constexpr ClassInfo kClasses[3] = {
    {"GUERRERO","Vanguardia","Maestro del combate cuerpo a cuerpo. Alto aguante y defensa.",
     nullptr, 120,15,8,20},
    {"MAGO","Arcano","Domina las artes magicas. Baja defensa pero poder devastador.",
     "mago.xp", 70,8,3,100},
    {"RANGER","Explorador","Agil y versatil. Experto en trampas, venenos y el arco.",
     nullptr, 90,12,5,50},
};

// Intenta cargar un retrato .xp; devuelve placeholder si no existe o falla.
static Element loadPortrait(const char* filename) {
    if (!filename) {
        return vbox({
            text("             ") ,
            text("   [sin      ") ,
            text("   retrato]  ") ,
            text("             ") ,
        }) | border | color(Color::GrayDark) | dim;
    }
    try {
        XpFile xp = loadXp(std::string(ASSETS_DIR) + "/art/" + filename);
        if (xp.layers.empty()) throw std::runtime_error("vacío");
        return xpToElementHalfBlock(xp.layers[0]) | border;
    } catch (...) {
        return vbox({
            text("  [retrato   ") ,
            text("   no        ") ,
            text("   encontrado]"),
        }) | border | color(Color::GrayDark) | dim;
    }
}

Element Renderer::drawClassSelect(int selection) {
    // ── Lista vertical de clases (izquierda) ──────────────────────────────────
    auto makeRow = [&](int i) -> Element {
        const auto& c = kClasses[i];
        bool sel = (i == selection);

        auto content = vbox({
            text(c.name) | hcenter | bold,
            text(std::string("[ ") + c.role + " ]") | hcenter,
            separator(),
            hbox({
                text("HP:")  | color(Color::Green), text(" " + std::to_string(c.hp))  | color(Color::Green),
                text("  ATK:") | color(Color::Green), text(" " + std::to_string(c.atk)) | color(Color::Green),
                text("  DEF:") | color(Color::Green), text(" " + std::to_string(c.def)) | color(Color::Green),
                text("  MP:")  | color(Color::Cyan),  text(" " + std::to_string(c.mana))| color(Color::Cyan),
            }) | hcenter,
            text(c.desc) | hcenter | dim,
        });

        if (sel) content = content | color(Color::Yellow) | bold;
        else     content = content | color(Color::White) | dim;
        return content | border | size(WIDTH, EQUAL, 40);
    };

    auto left = vbox({
        makeRow(0),
        text(""),
        makeRow(1),
        text(""),
        makeRow(2),
    }) | flex;

    // ── Retrato (derecha) ─────────────────────────────────────────────────────
    Element portrait = loadPortrait(kClasses[selection].portrait);

    return vbox({
        text("T E N E B R A R I U M") | hcenter | color(Color::Cyan) | bold,
        text("Elige tu clase:") | hcenter,
        text(""),
        hbox({left, separator(), portrait}),
        text(""),
        text("↑ ↓ para navegar  |  ENTER para confirmar  |  ESC para volver") | hcenter | dim,
    }) | center | border;
}

// ─── drawHudSelect ────────────────────────────────────────────────────────────

Element Renderer::drawHudSelect(int selection) {
    auto makePreview = [&](int idx, const char* label) -> Element {
        bool sel = (idx == selection);
        Element preview;
        if (idx == 0) {
            // Sidebar layout preview
            preview = hbox({
                vbox({
                    text("· · · · · · · · · ·"),
                    text("· · · · · · · · · ·"),
                    text("· · · · · @  · · ·"),
                    text("· · · · · · · · · ·"),
                    text("· · · · · · · · · ·"),
                }) | flex,
                separator(),
                vbox({
                    text("Hero"),
                    text("Lv.1"),
                    text(""),
                    text("HP██"),
                    text("MP░░"),
                }) | size(WIDTH, EQUAL, 6),
            }) | border;
        } else {
            // Bottom bar layout preview
            preview = vbox({
                vbox({
                    text("· · · · · · · · · · ·"),
                    text("· · · · · · · · · · ·"),
                    text("· · · · ·  @  · · · ·"),
                    text("· · · · · · · · · · ·"),
                }) | flex,
                separator(),
                text("Hero Lv1 HP██░  MP░░"),
            }) | border;
        }

        auto labelElem = text(label) | hcenter;
        if (sel) labelElem = labelElem | color(Color::Yellow) | bold;

        return vbox({preview, labelElem});
    };

    return vbox({
        text("T E N E B R A R I U M") | hcenter | color(Color::Cyan) | bold,
        text("Elige el estilo de interfaz:") | hcenter,
        text(""),
        hbox({makePreview(0, "Panel lateral"), separator(), makePreview(1, "Barra inferior")}) | hcenter,
        text(""),
        text("Flechas para navegar  |  ENTER para confirmar  |  ESC para volver") | hcenter | dim,
    }) | center | border;
}

// ─── makeMap ─────────────────────────────────────────────────────────────────

Element Renderer::makeMap(const Map& map, const std::vector<MapEntity>& entities) {
    Position playerPos = map.getPlayerPos();
    const int viewW = 60, viewH = 28;
    int camX = playerPos.x - viewW / 2;
    int camY = playerPos.y - viewH / 2;

    Elements rows;
    for (int sy = 0; sy < viewH; sy++) {
        Elements cells;
        for (int sx = 0; sx < viewW; sx++) {
            int mx = camX + sx;
            int my = camY + sy;

            if (mx < 0 || mx >= map.width() || my < 0 || my >= map.height()) {
                cells.push_back(text(" "));
                continue;
            }

            const Tile& tile = map.at(mx, my);
            if (!tile.explored) { cells.push_back(text(" ")); continue; }

            // Player glyph
            if (mx == playerPos.x && my == playerPos.y) {
                cells.push_back(text("@") | color(Color::Yellow) | bold);
                continue;
            }

            // Map entities (only if tile is currently visible)
            if (tile.visible) {
                bool drew = false;
                for (const auto& ent : entities) {
                    if (ent.pos.x == mx && ent.pos.y == my) {
                        Color col = colorFromPair(ent.colorPair);
                        auto e = text(std::string(1, ent.glyph)) | color(col);
                        if (ent.bold) e = e | bold;
                        cells.push_back(e);
                        drew = true;
                        break;
                    }
                }
                if (drew) continue;
            }

            // Secret wall: drawn as dim grey '#'
            if (tile.type == TileType::SecretWall) {
                cells.push_back(text("#") | color(Color::GrayDark));
                continue;
            }

            // Normal tile: bright if visible, dim if only explored
            std::string g(1, tile.glyph);
            if (tile.visible)
                cells.push_back(text(g) | color(Color::White));
            else
                cells.push_back(text(g) | color(Color::White) | dim);
        }
        rows.push_back(hbox(std::move(cells)));
    }
    return vbox(std::move(rows));
}

// ─── makeHudPanel (sidebar) ───────────────────────────────────────────────────

Element Renderer::makeHudPanel(const Player& player) {
    return vbox({
        text(player.getName().substr(0, 14)) | color(Color::Cyan) | bold,
        text(std::string("[") + className(player) + "]"),
        text("Nivel " + std::to_string(player.getLevel())),
        separator(),
        text("HP  " + std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp())) | color(Color::Green),
        makeStatBar(player.getHp(), player.getMaxHp(), Color::Green),
        text("MP  " + std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana())) | color(Color::Cyan),
        makeStatBar(player.getMana(), player.getMaxMana(), Color::Cyan),
        text("XP  " + std::to_string(player.getXp())) | color(Color::Yellow),
        separator(),
        text("ATK  " + std::to_string(player.getAttack())),
        text("DEF  " + std::to_string(player.getDefense())),
        separator(),
        text("EQUIPO") | bold,
        text("Arma    : " + (player.getEquippedWeapon()
            ? player.getEquippedWeapon()->name.substr(0, 12) : std::string("-")))
            | (player.getEquippedWeapon() ? color(Color::Yellow) : dim),
        text("Armadura: " + (player.getEquippedArmor()
            ? player.getEquippedArmor()->name.substr(0, 12) : std::string("-")))
            | (player.getEquippedArmor() ? color(Color::Green) : dim),
        separator(),
        text("Inventario") | bold,
        text(std::to_string(player.getInventory().usedSlots()) + "/" +
             std::to_string(player.getInventory().totalSlots()) + " slots"),
        makeStatBar(player.getInventory().usedSlots(), player.getInventory().totalSlots(), Color::White),
        separator(),
        text("$ " + std::to_string(player.getCoins()) + " monedas") | color(Color::Yellow) | bold,
        text("k " + std::to_string(player.getKeys()) + " llave(s)") | color(Color::Cyan),
        text("+ " + std::to_string(player.countConsumables()) + " pocion(es)") | color(Color::Green),
        text("Piso " + std::to_string(player.getDungeonFloor())) | dim,
        separator(),
        text("WASD  mover") | dim,
        text("  P   pocion") | dim,
        text("  I   mochila") | dim,
        text("  Q   salir") | dim,
    }) | border | size(WIDTH, EQUAL, 22);
}

// ─── makeHudBar (bottom bar) ──────────────────────────────────────────────────

Element Renderer::makeHudBar(const Player& player) {
    return vbox({
        separator(),
        hbox({
            text(player.getName().substr(0, 12)) | color(Color::Cyan) | bold,
            text(" [" + std::string(className(player)) + "] Nv." + std::to_string(player.getLevel())),
            text("  HP:") | color(Color::Green) | bold,
            text(std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp())) | color(Color::Green),
            text(" "), makeStatBar(player.getHp(), player.getMaxHp(), Color::Green),
            text("  MP:") | color(Color::Cyan) | bold,
            text(std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana())) | color(Color::Cyan),
        }),
        hbox({
            text("ATK:") | dim, text(std::to_string(player.getAttack())) | dim,
            text(" DEF:") | dim, text(std::to_string(player.getDefense())) | dim,
            text("  $") | color(Color::Yellow), text(std::to_string(player.getCoins())) | color(Color::Yellow),
            text(" k") | color(Color::Cyan), text(std::to_string(player.getKeys())) | color(Color::Cyan),
            text(" +") | color(Color::Green), text(std::to_string(player.countConsumables())) | color(Color::Green),
            text("  P") | dim, text(std::to_string(player.getDungeonFloor())) | dim,
            text("  WASD:mover  P:pocion  I:mochila  Q:salir") | dim,
        }),
    });
}

// ─── drawExploration ─────────────────────────────────────────────────────────

Element Renderer::drawExploration(const Map& map, const Player& player, HudLayout layout,
                                   const std::vector<MapEntity>& entities,
                                   const std::string& message) {
    if (layout == HudLayout::Sidebar)
        return drawExplorationSidebar(map, player, entities, message);
    return drawExplorationBottom(map, player, entities, message);
}

Element Renderer::drawExplorationSidebar(const Map& map, const Player& player,
                                          const std::vector<MapEntity>& entities,
                                          const std::string& message) {
    Element mapElem = makeMap(map, entities);
    if (!message.empty())
        mapElem = dbox({mapElem,
                        text(" " + message + " ") | color(Color::Yellow) | bold | inverted | hcenter | vcenter});

    return hbox({
        mapElem | flex,
        separator(),
        makeHudPanel(player),
    }) | border;
}

Element Renderer::drawExplorationBottom(const Map& map, const Player& player,
                                         const std::vector<MapEntity>& entities,
                                         const std::string& message) {
    Element mapElem = makeMap(map, entities);
    if (!message.empty())
        mapElem = dbox({mapElem,
                        text(" " + message + " ") | color(Color::Yellow) | bold | inverted | hcenter | vcenter});

    return vbox({
        mapElem | flex,
        makeHudBar(player),
    }) | border;
}

// ─── drawCombat ──────────────────────────────────────────────────────────────

Element Renderer::drawCombat(const CombatSystem& combat, const Player& player,
                              bool showingArts, int artSelection) {
    // ─── Enemies panel ────────────────────────────────────────────────────────
    Elements enemyElems;
    enemyElems.push_back(text("ENEMIGOS") | bold);
    enemyElems.push_back(separator());

    const auto& enemies = combat.getEnemies();
    for (int i = 0; i < static_cast<int>(enemies.size()); i++) {
        const auto& e = enemies[i];
        bool isTarget = (i == combat.getCurrentTarget());

        if (!e->isAlive()) {
            enemyElems.push_back(text("  " + e->getName() + " [MUERTO]") | dim);
            continue;
        }

        auto nameElem = text((isTarget ? "> " : "  ") + e->getName());
        if (isTarget) nameElem = nameElem | color(Color::Yellow) | bold;
        enemyElems.push_back(nameElem);

        // HP bar
        enemyElems.push_back(hbox({
            text("  HP " + std::to_string(e->getHp()) + "/" + std::to_string(e->getMaxHp()) + " ") | color(Color::Green),
            makeStatBar(e->getHp(), e->getMaxHp(), Color::Green),
        }));

        // Status effects
        const auto& efx = combat.getEnemyEffects(i);
        std::string tags;
        for (const auto& fx : efx) {
            switch (fx.type) {
                case StatusEffect::Type::Poisoned:    tags += "[VEN]"; break;
                case StatusEffect::Type::Frozen:      tags += "[HIE]"; break;
                case StatusEffect::Type::TrapPending: tags += "[TRP]"; break;
                default: break;
            }
        }
        if (!tags.empty())
            enemyElems.push_back(text("  " + tags) | color(Color::Yellow) | dim);

        enemyElems.push_back(text(""));
    }

    // ─── Log panel ────────────────────────────────────────────────────────────
    Elements logElems;
    logElems.push_back(text("LOG") | bold);
    logElems.push_back(separator());

    const auto& log = combat.getLog();
    int logStart = std::max(0, static_cast<int>(log.size()) - 15);
    for (int i = logStart; i < static_cast<int>(log.size()); i++)
        logElems.push_back(text(log[i]) | color(Color::White));

    // ─── Action Points display ────────────────────────────────────────────────
    int ap    = combat.getCurrentAp();
    int maxAp = combat.getMaxAp();

    Elements apDots;
    for (int i = 0; i < maxAp; i++) {
        if (i < ap)
            apDots.push_back(text("● ") | color(Color::Yellow) | bold);
        else
            apDots.push_back(text("○ ") | dim);
    }

    // Player status effects
    const auto& pfx = combat.getPlayerEffects();
    std::string ptags;
    for (const auto& fx : pfx) {
        switch (fx.type) {
            case StatusEffect::Type::Defending:      ptags += "[DEF]"; break;
            case StatusEffect::Type::DefendingHeavy: ptags += "[ESC]"; break;
            case StatusEffect::Type::AttackBoosted:  ptags += "[ATK+]"; break;
            default: break;
        }
    }

    auto playerBar = vbox({
        hbox({
            text(player.getName() + " [" + className(player) + "]") | color(Color::Cyan) | bold,
            text("  HP: ") | color(Color::Green) | bold,
            text(std::to_string(player.getHp()) + "/" + std::to_string(player.getMaxHp())) | color(Color::Green),
            text(" "), makeStatBar(player.getHp(), player.getMaxHp(), Color::Green),
            text("  MP: ") | color(Color::Cyan) | bold,
            text(std::to_string(player.getMana()) + "/" + std::to_string(player.getMaxMana())) | color(Color::Cyan),
            text(" "), makeStatBar(player.getMana(), player.getMaxMana(), Color::Cyan),
            text(ptags.empty() ? "" : "  " + ptags) | color(Color::Yellow),
        }),
        hbox(([&]{
            Elements r;
            r.push_back(text("PA: ") | bold);
            for (auto& d : apDots) r.push_back(d);
            r.push_back(text(" (" + std::to_string(ap) + "/" + std::to_string(maxAp) + " PA)") | dim);
            return r;
        })()),
    });

    // ─── Action / Arts menu ───────────────────────────────────────────────────
    Element actionMenu;
    if (!showingArts) {
        auto printAction = [&](const char* key, const char* desc, const char* cost, int required) -> Element {
            bool needsDim = (required > 0 && ap < required);
            auto e = text(std::string(key) + " " + desc + "  " + cost);
            if (needsDim) e = e | dim;
            return e;
        };

        Elements phase1;
        phase1.push_back(printAction("[A]", "Atacar",        "1 PA", 1));
        phase1.push_back(printAction("[F]", "Ataque Fuerte", "2 PA", 2));

        auto arts = player.getAvailableArts();
        int minCost = 99;
        for (const auto& a : arts) if (a.apCost < minCost) minCost = a.apCost;

        Elements phase2;
        phase2.push_back(printAction("[H]", "Habilidad", "var PA", minCost));
        phase2.push_back(printAction("[D]", "Defender",  "1 PA",   1));

        Elements phase3;
        phase3.push_back(printAction("[SPACE]", "Terminar turno", "0 PA", 0));
        phase3.push_back(printAction("[R]",     "Huir",           "3 PA", 3));

        Element hint = text("[TAB] Cambiar objetivo") | dim;

        Element extra = text("");
        if (combat.getPhase() == CombatPhase::EnemyTurn)
            extra = text("~~ Turno del enemigo ~~") | hcenter | color(Color::Cyan) | bold;
        if (combat.isOver())
            extra = text(combat.playerWon()
                ? "VICTORIA!  Cualquier tecla para continuar"
                : "DERROTA!   Cualquier tecla para continuar") | hcenter | color(Color::Yellow) | bold;

        actionMenu = vbox({
            hbox({vbox(std::move(phase1)) | flex, vbox(std::move(phase2)) | flex}),
            hbox({vbox(std::move(phase3)) | flex, hint | flex}),
            extra,
        });
    } else {
        // Arts submenu
        auto arts = player.getAvailableArts();
        Elements artRows;
        artRows.push_back(text("=== Habilidades ===  (ESC para volver)") | color(Color::Cyan) | bold);

        for (int i = 0; i < static_cast<int>(arts.size()); i++) {
            const auto& art = arts[i];
            bool canAfford = (ap >= art.apCost) && (player.getMana() >= art.manaCost);
            bool selected  = (i == artSelection);

            auto row = text("[" + std::to_string(i + 1) + "] " +
                            art.name + "  " +
                            std::to_string(art.apCost) + " PA  " +
                            std::to_string(art.manaCost) + " MP  " +
                            art.description);

            if (selected && canAfford) row = row | color(Color::Yellow) | bold | inverted;
            else if (selected)         row = row | bold | inverted;
            else if (!canAfford)       row = row | dim;

            artRows.push_back(row);
        }
        artRows.push_back(text("Flechas arriba/abajo  |  ENTER para confirmar") | dim);
        actionMenu = vbox(std::move(artRows));
    }

    return vbox({
        hbox({
            vbox(std::move(enemyElems)) | flex,
            separator(),
            vbox(std::move(logElems)) | flex,
        }) | flex,
        separator(),
        playerBar,
        separator(),
        actionMenu,
    }) | border;
}

// ─── stub screens ────────────────────────────────────────────────────────────

Element Renderer::drawInventory(const Player& player, int selection) {
    Elements rows;

    // ── Equipped slots (rows 0, 1) ────────────────────────────────────────────
    rows.push_back(text("=== EQUIPO ===") | bold | color(Color::Cyan));

    auto makeSlot = [&](int row, const char* label, const std::optional<Item>& slot) {
        std::string line;
        if (slot) {
            std::string tag = (slot->type == ItemType::Weapon) ? "[ARMA]   " : "[ARMADURA]";
            line = tag + " " + slot->name + "  +" + std::to_string(slot->statBonus)
                 + (slot->type == ItemType::Weapon ? " ATK" : " DEF");
        } else {
            line = std::string(label) + "  (vacio)";
        }
        auto e = text(line);
        if (slot) e = e | color(slot->type == ItemType::Weapon ? Color::Yellow : Color::Green);
        else      e = e | dim;
        if (selection == row) e = e | inverted;
        return e;
    };

    rows.push_back(makeSlot(0, "Arma    ", player.getEquippedWeapon()));
    rows.push_back(makeSlot(1, "Armadura", player.getEquippedArmor()));
    rows.push_back(separator());

    // ── Bag items (rows 2+) ───────────────────────────────────────────────────
    rows.push_back(text("=== MOCHILA ===") | bold | color(Color::Cyan));

    const auto& items = player.getInventory().items();
    if (items.empty()) {
        rows.push_back(text("  (vacia)") | dim);
    } else {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            const auto& item = items[i];
            int row = 2 + i;

            std::string tag;
            Color col = Color::White;
            switch (item.type) {
                case ItemType::Weapon:
                    tag = "[ARMA]   "; col = Color::Yellow; break;
                case ItemType::Armor:
                    tag = "[ARMADURA]"; col = Color::Green; break;
                case ItemType::Consumable:
                    tag = "[POCION] "; col = Color::Cyan; break;
                default:
                    tag = "[MISC]   "; break;
            }

            std::string line = tag + " " + item.name;
            if (item.type != ItemType::Consumable)
                line += "  +" + std::to_string(item.statBonus)
                      + (item.type == ItemType::Weapon ? " ATK" : " DEF");
            else
                line += "  +" + std::to_string(item.statBonus) + " HP";

            auto e = text(line) | color(col);
            if (selection == row) e = e | inverted;
            rows.push_back(e);
        }
    }

    rows.push_back(separator());

    // ── Stats footer ──────────────────────────────────────────────────────────
    rows.push_back(hbox({
        text("ATK:" + std::to_string(player.getAttack())) | color(Color::Green),
        text("  DEF:" + std::to_string(player.getDefense())) | color(Color::Green),
        text("  $" + std::to_string(player.getCoins())) | color(Color::Yellow),
        text("  Slots:" + std::to_string(player.getInventory().usedSlots())
             + "/" + std::to_string(player.getInventory().totalSlots())) | dim,
    }));
    rows.push_back(text("[↑↓] Navegar  [E] Equipar  [U] Usar  [ESC] Cerrar") | dim);

    return vbox(std::move(rows)) | border | center;
}

Element Renderer::drawQuestLog() {
    return text("[Misiones] - en construccion") | border;
}

Element Renderer::drawGameOver() {
    return vbox({
        text("G A M E   O V E R") | hcenter | color(Color::Yellow) | bold,
        text(""),
        text("ENTER para volver al menu") | hcenter | dim,
    }) | center | border;
}

// ─── shop screen ─────────────────────────────────────────────────────────────

Element Renderer::drawShop(const std::vector<ShopItem>& stock,
                            int selection, const Player& player,
                            const std::string& message) {
    Color orange = Color::RGB(255, 165, 0);

    Elements rows;
    rows.push_back(text("=== TIENDA DEL PISO ===") | hcenter | color(orange) | bold);
    rows.push_back(text("$ " + std::to_string(player.getCoins()) + " monedas disponibles")
                   | color(orange));
    rows.push_back(separator());

    for (int i = 0; i < static_cast<int>(stock.size()); i++) {
        const auto& s = stock[i];
        bool sel = (i == selection);
        std::string prefix = s.sold ? "[VENDIDO] " : "          ";
        std::string line   = prefix + s.item.name
                           + "  -  " + std::to_string(s.price) + " $"
                           + "   " + s.item.description;
        auto e = text(line);
        if      (s.sold) e = e | dim;
        else if (sel)    e = e | color(orange) | bold | inverted;
        else             e = e | color(Color::White);
        rows.push_back(e);
    }

    rows.push_back(separator());
    if (!message.empty())
        rows.push_back(text(message) | color(Color::Green) | bold);
    rows.push_back(text(""));
    rows.push_back(text("- navegar  |  ENTER comprar  |  ESC salir") | hcenter | dim);

    return vbox(std::move(rows)) | color(orange) | center | border;
}
