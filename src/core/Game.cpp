#include "Game.hpp"
#include "ui/Renderer.hpp"
#include "entities/Enemy.hpp"
#include <ncurses.h>
#include <locale.h>

// Forward declarations of file-local helpers (defined near setState)
static char glyphForEnemy(EnemyType t);
static std::unique_ptr<Enemy> makeEnemy(EnemyType t);
static int  xpForEnemy(EnemyType t);
static Item pickWeapon(PlayerClass cls);
static Item pickArmor(PlayerClass cls);

// Color pair indices (defined here, used by Renderer)
// 1 = normal, 2 = selected/highlight, 3 = title, 4 = stats, 5 = selected box
static void initColors() {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE,  -1);           // normal
    init_pair(2, COLOR_YELLOW, -1);           // highlighted option
    init_pair(3, COLOR_CYAN,   -1);           // title
    init_pair(4, COLOR_GREEN,  -1);           // stats
    init_pair(5, COLOR_BLACK,  COLOR_CYAN);   // selected class box
    init_pair(6, COLOR_RED,     -1);          // enemies
    // Secret walls: mid-grey (244 in xterm-256) or dim white as fallback
    init_pair(7, COLORS >= 256 ? 244 : COLOR_WHITE, -1);
}

Game::Game()
    : state_(GameState::MainMenu),
      menuPhase_(MenuPhase::Title),
      running_(true),
      menuSelection_(0),
      classSelection_(0),
      hudSelection_(0),
      hudLayout_(HudLayout::Sidebar),
      combatShowingArts_(false),
      combatArtSelection_(0),
      combatWorldEnemyIdx_(-1),
      lockedDoorPos_({0,0}),
      lockedDoorExists_(false),
      lockedDoorOpen_(false),
      stairsPos_({0,0})
{
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    initColors();
}

Game::~Game() {
    endwin();
}

void Game::run() {
    while (running_) {
        render();
        int key = getch();
        processInput(key);
        update();
    }
}

void Game::processInput(int key) {
    switch (state_) {
        case GameState::MainMenu:
            switch (menuPhase_) {
                case MenuPhase::Title:       inputTitle(key);       break;
                case MenuPhase::NameInput:   inputNameInput(key);   break;
                case MenuPhase::ClassSelect: inputClassSelect(key); break;
                case MenuPhase::HudSelect:   inputHudSelect(key);   break;
            }
            break;
        case GameState::Exploration: {
            explorationMsg_.clear();
            if (key == 'q' || key == 'Q') { running_ = false; break; }
            if (!map_) break;
            Position pos = map_->getPlayerPos();

            // Search adjacent tiles for secret walls
            if (key == 'e' || key == 'E') {
                const int sdx[] = {0, 0, -1, 1};
                const int sdy[] = {-1, 1, 0, 0};
                bool found = false;
                for (int i = 0; i < 4; i++) {
                    int ax = pos.x + sdx[i], ay = pos.y + sdy[i];
                    if (map_->isSecretWall(ax, ay)) {
                        map_->revealSecretWall(ax, ay);
                        map_->updateFov();
                        explorationMsg_ = "Encontraste una sala secreta!";
                        found = true;
                    }
                }
                if (!found) explorationMsg_ = "No hay nada aqui...";
                break;
            }

            int nx = pos.x, ny = pos.y;
            if      (key == KEY_UP    || key == 'w' || key == 'k') ny--;
            else if (key == KEY_DOWN  || key == 's' || key == 'j') ny++;
            else if (key == KEY_LEFT  || key == 'a' || key == 'h') nx--;
            else if (key == KEY_RIGHT || key == 'd' || key == 'l') nx++;
            if (!map_->isWalkable(nx, ny)) break;

            // Locked door blocks movement
            if (lockedDoorExists_ && !lockedDoorOpen_ &&
                lockedDoorPos_.x == nx && lockedDoorPos_.y == ny) {
                if (player_->useKey()) {
                    lockedDoorOpen_ = true;
                    explorationMsg_ = "Usas una llave. La puerta se abre y aparecen unas escaleras!";
                } else {
                    explorationMsg_ = "La puerta esta cerrada con llave. Necesitas una llave.";
                }
                break;
            }

            // Enemy collision → combat
            bool triggered = false;
            for (int i = 0; i < static_cast<int>(worldEnemies_.size()); i++) {
                auto& we = worldEnemies_[i];
                if (we.alive && we.pos.x == nx && we.pos.y == ny) {
                    combatWorldEnemyIdx_ = i;
                    setState(GameState::Combat);
                    triggered = true;
                    break;
                }
            }
            if (triggered) break;

            // Move player
            map_->setPlayerPos(nx, ny);
            map_->updateFov();

            // Chest on new tile
            for (auto& ch : worldChests_) {
                if (!ch.opened && ch.pos.x == nx && ch.pos.y == ny) {
                    openChest(ch);
                    break;
                }
            }

            // Stairs on new tile
            if ((!lockedDoorExists_ || lockedDoorOpen_) &&
                stairsPos_.x == nx && stairsPos_.y == ny) {
                player_->descendFloor();
                setState(GameState::Exploration);
                explorationMsg_ = "Desciendes al piso " +
                                  std::to_string(player_->getDungeonFloor()) + "...";
            }
            break;
        }
        case GameState::Combat:
            inputCombat(key);
            break;
        case GameState::Inventory:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::QuestLog:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::GameOver:
            if (key == '\n' || key == KEY_ENTER)
                setState(GameState::MainMenu);
            break;
    }
}

void Game::inputTitle(int key) {
    switch (key) {
        case KEY_UP:
        case KEY_LEFT:
        case KEY_DOWN:
        case KEY_RIGHT:
            menuSelection_ = (menuSelection_ + 1) % 2;
            break;
        case '\n':
        case KEY_ENTER:
            if (menuSelection_ == 1) {
                running_ = false;
            } else {
                playerName_.clear();
                menuPhase_ = MenuPhase::NameInput;
                curs_set(1);
            }
            break;
        case 'q':
        case 'Q':
            running_ = false;
            break;
    }
}

void Game::inputNameInput(int key) {
    if (key == '\n' || key == KEY_ENTER) {
        if (!playerName_.empty()) {
            classSelection_ = 0;
            menuPhase_ = MenuPhase::ClassSelect;
            curs_set(0);
        }
        return;
    }
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        if (!playerName_.empty()) playerName_.pop_back();
        return;
    }
    if (key == 27) {
        menuPhase_ = MenuPhase::Title;
        curs_set(0);
        return;
    }
    if (key >= 32 && key <= 126 && static_cast<int>(playerName_.size()) < 16)
        playerName_ += static_cast<char>(key);
}

void Game::inputClassSelect(int key) {
    switch (key) {
        case KEY_LEFT:
            classSelection_ = (classSelection_ + 2) % 3;
            break;
        case KEY_RIGHT:
            classSelection_ = (classSelection_ + 1) % 3;
            break;
        case 27:
            menuPhase_ = MenuPhase::NameInput;
            curs_set(1);
            break;
        case '\n':
        case KEY_ENTER:
            hudSelection_ = 0;
            menuPhase_ = MenuPhase::HudSelect;
            break;
    }
}

void Game::inputHudSelect(int key) {
    switch (key) {
        case KEY_LEFT:
        case KEY_RIGHT:
            hudSelection_ = (hudSelection_ + 1) % 2;
            break;
        case 27:
            menuPhase_ = MenuPhase::ClassSelect;
            break;
        case '\n':
        case KEY_ENTER: {
            hudLayout_ = (hudSelection_ == 0) ? HudLayout::Sidebar : HudLayout::Bottom;
            PlayerClass cls;
            switch (classSelection_) {
                case 0: cls = PlayerClass::Warrior; break;
                case 1: cls = PlayerClass::Mage;    break;
                default: cls = PlayerClass::Ranger; break;
            }
            player_ = std::make_unique<Player>(playerName_, cls);
            setState(GameState::Exploration);
            break;
        }
    }
}

void Game::update() {
    // per-frame game logic (placeholder)
}

void Game::render() {
    clear();

    switch (state_) {
        case GameState::MainMenu:
            switch (menuPhase_) {
                case MenuPhase::Title:
                    Renderer::drawTitle(menuSelection_);
                    break;
                case MenuPhase::NameInput:
                    Renderer::drawNameInput(playerName_);
                    break;
                case MenuPhase::ClassSelect:
                    Renderer::drawClassSelect(classSelection_);
                    break;
                case MenuPhase::HudSelect:
                    Renderer::drawHudSelect(hudSelection_);
                    break;
            }
            break;
        case GameState::Exploration:
            if (map_ && player_) {
                std::vector<MapEntity> entities;
                // Enemies — red
                for (const auto& we : worldEnemies_)
                    if (we.alive)
                        entities.push_back({we.pos, glyphForEnemy(we.type), 6, true});
                // Chests (unopened) — yellow
                for (const auto& ch : worldChests_)
                    if (!ch.opened)
                        entities.push_back({ch.pos, '$', 2, true});
                // Locked door — white dim
                if (lockedDoorExists_ && !lockedDoorOpen_)
                    entities.push_back({lockedDoorPos_, '+', 1, false});
                // Stairs — cyan (only accessible when door is open or no door)
                if (!lockedDoorExists_ || lockedDoorOpen_)
                    entities.push_back({stairsPos_, '>', 3, true});
                Renderer::drawExploration(*map_, *player_, hudLayout_,
                                          entities, explorationMsg_);
            }
            break;
        case GameState::Combat:
            if (combat_ && player_)
                Renderer::drawCombat(*combat_, *player_, combatShowingArts_, combatArtSelection_);
            break;
        case GameState::Inventory:
            Renderer::drawInventory();
            break;
        case GameState::QuestLog:
            Renderer::drawQuestLog();
            break;
        case GameState::GameOver:
            Renderer::drawGameOver();
            break;
    }

    refresh();
}

// ─── helpers ─────────────────────────────────────────────────────────────────

static char glyphForEnemy(EnemyType t) {
    switch (t) {
        case EnemyType::Goblin:   return 'g';
        case EnemyType::Skeleton: return 's';
        case EnemyType::Orc:      return 'o';
    }
    return '?';
}

static std::unique_ptr<Enemy> makeEnemy(EnemyType t) {
    switch (t) {
        case EnemyType::Goblin:
            return std::make_unique<Enemy>("Goblin",   125, 17, 4, 60, t, 1);
        case EnemyType::Skeleton:
            return std::make_unique<Enemy>("Esqueleto",120, 16, 8, 60, t, 1);
        case EnemyType::Orc:
            return std::make_unique<Enemy>("Orco",     140, 20, 10, 120, t, 1);
    }
    return std::make_unique<Enemy>("???", 10, 5, 1, 5, t, 1);
}

static int xpForEnemy(EnemyType t) {
    switch (t) {
        case EnemyType::Goblin:   return 20;
        case EnemyType::Skeleton: return 15;
        case EnemyType::Orc:      return 30;
    }
    return 5;
}

// ─── state transitions ────────────────────────────────────────────────────────

void Game::setState(GameState newState) {
    if (newState == GameState::MainMenu) {
        menuPhase_      = MenuPhase::Title;
        menuSelection_  = 0;
        classSelection_ = 0;
        hudSelection_   = 0;
        playerName_.clear();
        worldEnemies_.clear();
        combat_.reset();
        curs_set(0);
    }

    if (newState == GameState::Exploration) {
        worldEnemies_.clear();
        worldChests_.clear();
        lockedDoorExists_ = false;
        lockedDoorOpen_   = false;
        combat_.reset();

        map_ = std::make_unique<Map>(80, 40);
        BSPDungeon gen(80, 40);
        gen.generate(*map_);

        const auto& rooms = gen.getRooms();
        int n = static_cast<int>(rooms.size());

        map_->setPlayerPos(rooms[0].centerX(), rooms[0].centerY());
        map_->updateFov();

        // Tracks taken positions to avoid overlaps
        std::vector<Position> taken;
        taken.push_back({ rooms[0].centerX(), rooms[0].centerY() });

        // Pick a random free tile inside a room
        auto pickPos = [&](int ri) -> Position {
            const auto& r = rooms[ri];
            int iw = std::max(1, r.w - 2);
            int ih = std::max(1, r.h - 2);
            for (int attempt = 0; attempt < 30; attempt++) {
                Position p = { r.x + 1 + std::rand() % iw,
                               r.y + 1 + std::rand() % ih };
                bool ok = true;
                for (const auto& t : taken) if (t.x == p.x && t.y == p.y) { ok = false; break; }
                if (ok) { taken.push_back(p); return p; }
            }
            Position c = { r.centerX(), r.centerY() };
            taken.push_back(c);
            return c;
        };

        // Stairs in last room
        stairsPos_ = pickPos(n - 1);

        // Locked door in second-to-last room (if ≥ 3 rooms)
        if (n >= 3) {
            lockedDoorPos_    = pickPos(n - 2);
            lockedDoorExists_ = true;
        }

        // Enemies: ~50 % chance per non-start room, random type and position
        for (int i = 1; i < n; i++) {
            if (std::rand() % 2 == 0) {
                EnemyType t = static_cast<EnemyType>(std::rand() % 3);
                worldEnemies_.push_back({ pickPos(i), t, true });
            }
        }

        // Chests: 2–4 per floor in random rooms (skip start room and stairs room)
        PlayerClass cls = player_ ? player_->getClass() : PlayerClass::Warrior;
        int chestCount = 2 + std::rand() % 3;

        // Build a shuffled list of eligible rooms (not start, not last)
        std::vector<int> eligible;
        for (int i = 1; i < n - 1; i++) eligible.push_back(i);
        for (int i = static_cast<int>(eligible.size()) - 1; i > 0; i--)
            std::swap(eligible[i], eligible[std::rand() % (i + 1)]);

        chestCount = std::min(chestCount, static_cast<int>(eligible.size()));
        bool keyInChest = false;

        for (int ci = 0; ci < chestCount; ci++) {
            int roll = std::rand() % 100;
            ChestLoot loot; int coins = 0; Item item{};

            if (roll < 50)       { loot = ChestLoot::Coins; coins = 1 + std::rand() % 50; }
            else if (roll < 90)  { loot = ChestLoot::Item;
                                   item = (std::rand() % 2 == 0) ? pickWeapon(cls) : pickArmor(cls); }
            else                 { loot = ChestLoot::Key; keyInChest = true; }

            worldChests_.push_back({ pickPos(eligible[ci]), false, loot, coins, item });
        }

        // Guarantee a key source: if no chest has one, force one random chest to carry it
        // (enemies can also drop keys on kill, so this is just a safety net)
        if (lockedDoorExists_ && !keyInChest && !worldChests_.empty()) {
            int idx = std::rand() % static_cast<int>(worldChests_.size());
            worldChests_[idx] = { worldChests_[idx].pos, false, ChestLoot::Key, 0, {} };
        }

        // Place 1–2 secret rooms hidden behind SecretWall tiles
        int numSecret = 1 + std::rand() % 2;
        for (int i = 0; i < numSecret; i++)
            tryPlaceSecretRoom(taken, cls);
    }

    if (newState == GameState::Combat && player_) {
        combatShowingArts_  = false;
        combatArtSelection_ = 0;

        std::vector<std::unique_ptr<Enemy>> enemies;
        if (combatWorldEnemyIdx_ >= 0) {
            // Fight the specific world enemy the player walked into
            enemies.push_back(makeEnemy(worldEnemies_[combatWorldEnemyIdx_].type));
        } else {
            // Debug fight: Goblin + Esqueleto
            enemies.push_back(makeEnemy(EnemyType::Goblin));
            enemies.push_back(makeEnemy(EnemyType::Skeleton));
        }
        combat_ = std::make_unique<CombatSystem>(*player_, std::move(enemies));
    }

    state_ = newState;
}

void Game::openChest(WorldChest& chest) {
    chest.opened = true;
    switch (chest.loot) {
        case ChestLoot::Coins:
            player_->addCoins(chest.coins);
            explorationMsg_ = "Cofre: +" + std::to_string(chest.coins) + " monedas de oro!";
            break;
        case ChestLoot::Item:
            player_->applyItemBonus(chest.item);
            explorationMsg_ = "Cofre: encontraste " + chest.item.name + "!  (+"
                + std::to_string(chest.item.statBonus)
                + (chest.item.type == ItemType::Weapon ? " ATK" : " DEF") + ")";
            break;
        case ChestLoot::Key:
            player_->addKey();
            explorationMsg_ = "Cofre: encontraste una llave!";
            break;
    }
}

static Item pickWeapon(PlayerClass cls) {
    static const Item w[3][3] = {
        // Warrior
        { {"Espada Corta",    "Un filo confiable.",   ItemType::Weapon, 30, 1, 3},
          {"Hacha de Mano",   "Golpea con fuerza.",   ItemType::Weapon, 50, 1, 5},
          {"Mandoble",        "Lenta pero letal.",    ItemType::Weapon, 80, 2, 8} },
        // Mage
        { {"Varita de Roble", "Canaliza magia.",      ItemType::Weapon, 30, 1, 2},
          {"Baculo de Cristal","Poder arcano.",        ItemType::Weapon, 50, 1, 4},
          {"Grimorio Oscuro",  "Magia devastadora.",  ItemType::Weapon, 80, 2, 6} },
        // Ranger
        { {"Arco Corto",      "Rapido y preciso.",    ItemType::Weapon, 30, 1, 3},
          {"Arco Largo",      "Mayor alcance.",       ItemType::Weapon, 50, 1, 5},
          {"Ballesta",        "Poderosa y lenta.",    ItemType::Weapon, 80, 2, 7} },
    };
    int ci = (cls == PlayerClass::Warrior) ? 0 : (cls == PlayerClass::Mage) ? 1 : 2;
    return w[ci][std::rand() % 3];
}

static Item pickArmor(PlayerClass cls) {
    static const Item a[3][2] = {
        // Warrior
        { {"Cota de Malla",      "Proteccion media.",  ItemType::Armor, 40, 1, 3},
          {"Armadura de Placas", "Muy resistente.",    ItemType::Armor, 70, 2, 6} },
        // Mage
        { {"Tunica Arcana",  "Ligera y magica.",       ItemType::Armor, 30, 1, 1},
          {"Manto Mistico",  "Deflecta hechizos.",     ItemType::Armor, 60, 1, 3} },
        // Ranger
        { {"Cuero Reforzado",   "Agil y resistente.", ItemType::Armor, 35, 1, 2},
          {"Armadura de Cuero", "Balance perfecto.",   ItemType::Armor, 55, 1, 4} },
    };
    int ci = (cls == PlayerClass::Warrior) ? 0 : (cls == PlayerClass::Mage) ? 1 : 2;
    return a[ci][std::rand() % 2];
}

void Game::tryPlaceSecretRoom(std::vector<Position>& taken, PlayerClass cls) {
    // Scan for floor tiles whose adjacent wall could become a secret entrance.
    // Beyond the entrance wall we try to carve a INNER×INNER room in solid wall space.
    static const int DIRS[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    static const int INNER = 3;  // 3×3 inner room (total footprint 5×5 with border)

    struct Candidate { int wallX, wallY, dx, dy; };
    std::vector<Candidate> cands;

    for (int y = 2; y < map_->height() - 2; y++) {
        for (int x = 2; x < map_->width() - 2; x++) {
            if (map_->at(x, y).type != TileType::Floor) continue;
            for (auto& d : DIRS) {
                int wx = x + d[0], wy = y + d[1];
                if (wx < 1 || wx >= map_->width()-1 ||
                    wy < 1 || wy >= map_->height()-1) continue;
                if (map_->at(wx, wy).type == TileType::Wall)
                    cands.push_back({wx, wy, d[0], d[1]});
            }
        }
    }

    // Shuffle candidates
    for (int i = static_cast<int>(cands.size()) - 1; i > 0; i--)
        std::swap(cands[i], cands[std::rand() % (i + 1)]);

    for (const auto& c : cands) {
        // Compute the bounding box of the INNER×INNER room interior
        int startX, startY, endX, endY;
        if (c.dx != 0) {
            startX = c.wallX + (c.dx > 0 ?  1 : -INNER);
            endX   = c.wallX + (c.dx > 0 ?  INNER : -1);
            startY = c.wallY - INNER / 2;
            endY   = c.wallY + INNER / 2;
        } else {
            startX = c.wallX - INNER / 2;
            endX   = c.wallX + INNER / 2;
            startY = c.wallY + (c.dy > 0 ?  1 : -INNER);
            endY   = c.wallY + (c.dy > 0 ?  INNER : -1);
        }

        if (startX < 1 || endX >= map_->width()-1 ||
            startY < 1 || endY >= map_->height()-1) continue;

        // Every tile in the border ring (except the entrance) must be Wall
        bool valid = true;
        for (int by = startY - 1; by <= endY + 1 && valid; by++) {
            for (int bx = startX - 1; bx <= endX + 1 && valid; bx++) {
                if (bx == c.wallX && by == c.wallY) continue;  // entrance
                if (map_->at(bx, by).type != TileType::Wall) valid = false;
            }
        }
        if (!valid) continue;

        // Carve the room interior
        for (int y = startY; y <= endY; y++)
            for (int x = startX; x <= endX; x++)
                map_->at(x, y) = { TileType::Floor, '.', false, false };

        // Replace entrance wall with a SecretWall (glyph stays '#')
        map_->at(c.wallX, c.wallY).type = TileType::SecretWall;

        // Place guaranteed good loot: one item chest + coins
        Position itemPos  = { startX + 1, startY + 1 };
        Position coinsPos = { endX - 1,   endY - 1   };
        taken.push_back(itemPos);
        Item item = (std::rand() % 2 == 0) ? pickWeapon(cls) : pickArmor(cls);
        worldChests_.push_back({ itemPos, false, ChestLoot::Item, 0, item });

        if (coinsPos.x != itemPos.x || coinsPos.y != itemPos.y) {
            taken.push_back(coinsPos);
            int coins = 20 + std::rand() % 31;  // 20–50 coins
            worldChests_.push_back({ coinsPos, false, ChestLoot::Coins, coins, {} });
        }

        return;  // one secret room is enough per call
    }
}

void Game::returnToExploration() {
    // Return to the same map without regenerating — just clear combat state
    combat_.reset();
    combatWorldEnemyIdx_ = -1;
    state_ = GameState::Exploration;
}

void Game::inputCombat(int key) {
    if (!combat_) return;

    // If combat is over, any key returns to exploration or game over
    if (combat_->isOver()) {
        if (combat_->playerFled()) {
            returnToExploration();
        } else if (combat_->playerWon()) {
            if (combatWorldEnemyIdx_ >= 0) {
                auto& we = worldEnemies_[combatWorldEnemyIdx_];
                player_->gainXp(xpForEnemy(we.type));
                we.alive = false;
                // 15 % chance the enemy drops a key
                if (std::rand() % 100 < 15) {
                    player_->addKey();
                    explorationMsg_ = "El enemigo solto una llave!";
                }
            }
            returnToExploration();
        } else {
            setState(GameState::GameOver);
        }
        return;
    }

    if (combatShowingArts_) {
        // Arts submenu navigation
        auto arts = player_->getAvailableArts();
        int n = static_cast<int>(arts.size());
        switch (key) {
            case KEY_UP:
                combatArtSelection_ = (combatArtSelection_ - 1 + n) % n;
                break;
            case KEY_DOWN:
                combatArtSelection_ = (combatArtSelection_ + 1) % n;
                break;
            case '\n':
            case KEY_ENTER:
                combat_->doArt(combatArtSelection_);
                combatShowingArts_ = false;
                break;
            case 27:  // ESC
                combatShowingArts_ = false;
                break;
        }
        return;
    }

    // Main action menu
    switch (key) {
        case 'a': case 'A':
            combat_->doAttack();
            break;
        case 'f': case 'F':
            combat_->doHeavyAttack();
            break;
        case 'h': case 'H':
            combatShowingArts_  = true;
            combatArtSelection_ = 0;
            break;
        case 'd': case 'D':
            combat_->doDefend();
            break;
        case ' ':
            combat_->doEndTurn();
            break;
        case 'r': case 'R':
            combat_->doFlee();
            break;
        case '\t':  // TAB
            combat_->cycleTarget();
            break;
    }
}
