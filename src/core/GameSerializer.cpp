// GameSerializer — serialización/deserialización del estado completo del juego.
// Formato: texto plano, un campo por línea o separados por espacios.
//   Cabecera: "1\n" (número de versión — si cambia el formato, incrementar aquí y en load)
//   Strings: se guardan con prefijo de longitud ("5 hola " en vez de "hola") para
//            soportar strings con espacios (wstr/rstr).
// El archivo se guarda en <directorio_ejecutable>/saves/save.dat.
// Para añadir un campo nuevo: añadirlo al final de save() y load() para no romper
// partidas guardadas existentes con versiones anteriores.

#include "GameSerializer.hpp"
#include "Game.hpp"
#include <raylib.h>
#include <fstream>
#include <filesystem>
#include <sstream>

// ─── helpers ──────────────────────────────────────────────────────────────────

std::string GameSerializer::savePath()
{
    return std::string(GetApplicationDirectory()) + "saves/save.dat";
}

bool GameSerializer::hasSave()
{
    return std::ifstream(savePath()).good();
}

void GameSerializer::wstr(std::ostream& o, const std::string& s)
{
    o << s.size() << ' ';
    o.write(s.data(), static_cast<std::streamsize>(s.size()));
    o << ' ';
}

bool GameSerializer::rstr(std::istream& in, std::string& s)
{
    size_t n;
    if (!(in >> n)) return false;
    char sp;
    in.get(sp);
    s.resize(n);
    return n == 0 || in.read(&s[0], static_cast<std::streamsize>(n)).good();
}

void GameSerializer::witem(std::ostream& o, const Item& item)
{
    wstr(o, item.name);
    wstr(o, item.description);
    o << static_cast<int>(item.type) << ' ' << item.value << ' '
      << item.slots << ' ' << item.statBonus << '\n';
}

bool GameSerializer::ritem(std::istream& in, Item& item)
{
    if (!rstr(in, item.name) || !rstr(in, item.description))
        return false;
    int t;
    if (!(in >> t >> item.value >> item.slots >> item.statBonus))
        return false;
    item.type = static_cast<ItemType>(t);
    return true;
}

static char glyphForTile(TileType t)
{
    switch (t) {
    case TileType::Wall:       return '#';
    case TileType::SecretWall: return '#';
    case TileType::Floor:      return '.';
    case TileType::Stairs:     return '>';
    }
    return ' ';
}

// ─── save ─────────────────────────────────────────────────────────────────────

void GameSerializer::save(Game& g)
{
    std::filesystem::create_directories(
        std::string(GetApplicationDirectory()) + "saves");

    std::ofstream f(savePath());
    if (!f || !g.player_ || !g.map_)
        return;

    f << 1 << '\n'; // version

    // Player
    wstr(f, g.player_->name_);
    f << '\n' << static_cast<int>(g.player_->class_) << '\n';
    f << g.player_->level_ << ' ' << g.player_->xp_ << ' ' << g.player_->xpToNextLevel_ << ' '
      << g.player_->hp_ << ' ' << g.player_->maxHp_ << ' '
      << g.player_->mana_ << ' ' << g.player_->maxMana_ << ' '
      << g.player_->coins_ << ' ' << g.player_->dungeonFloor_ << ' '
      << g.player_->attack_ << ' ' << g.player_->defense_ << ' '
      << g.player_->baseAttack_ << ' ' << g.player_->baseDefense_ << '\n';

    f << (g.player_->equippedWeapon_.has_value() ? 1 : 0) << '\n';
    if (g.player_->equippedWeapon_)
        witem(f, *g.player_->equippedWeapon_);

    f << (g.player_->equippedArmor_.has_value() ? 1 : 0) << '\n';
    if (g.player_->equippedArmor_)
        witem(f, *g.player_->equippedArmor_);

    const auto& invItems = g.player_->getInventory().items();
    f << invItems.size() << '\n';
    for (const auto& item : invItems)
        witem(f, item);

    // Map
    f << g.map_->width() << ' ' << g.map_->height() << '\n';
    Position pp = g.map_->getPlayerPos();
    f << pp.x << ' ' << pp.y << '\n';
    for (int y = 0; y < g.map_->height(); y++) {
        for (int x = 0; x < g.map_->width(); x++) {
            const Tile& t = g.map_->at(x, y);
            f << static_cast<int>(t.type) << ' '
              << static_cast<int>(t.explored) << ' '
              << static_cast<int>(t.visible);
            f << (x < g.map_->width() - 1 ? ' ' : '\n');
        }
    }

    // World enemies
    f << g.worldEnemies_.size() << '\n';
    for (const auto& we : g.worldEnemies_)
        f << we.pos.x << ' ' << we.pos.y << ' '
          << we.spawnPos.x << ' ' << we.spawnPos.y << ' '
          << static_cast<int>(we.type) << ' '
          << static_cast<int>(we.alive) << ' '
          << static_cast<int>(we.isBoss) << '\n';

    // World chests
    f << g.worldChests_.size() << '\n';
    for (const auto& ch : g.worldChests_) {
        f << ch.pos.x << ' ' << ch.pos.y << ' '
          << static_cast<int>(ch.opened) << ' '
          << static_cast<int>(ch.loot) << ' '
          << ch.coins << ' ';
        witem(f, ch.item);
    }

    // Locked door
    f << static_cast<int>(g.lockedDoorExists_) << ' ';
    if (g.lockedDoorExists_)
        f << g.lockedDoorPos_.x << ' ' << g.lockedDoorPos_.y << ' '
          << static_cast<int>(g.lockedDoorOpen_);
    f << '\n';

    // Stairs
    f << g.stairsPos_.x << ' ' << g.stairsPos_.y << '\n';

    // Shop
    f << static_cast<int>(g.shopExists_) << '\n';
    if (g.shopExists_)
        f << g.shopMerchantPos_.x << ' ' << g.shopMerchantPos_.y << ' '
          << g.shopRoom_.x << ' ' << g.shopRoom_.y << ' '
          << g.shopRoom_.w << ' ' << g.shopRoom_.h << '\n';
    f << g.shopStock_.size() << '\n';
    for (const auto& s : g.shopStock_) {
        f << static_cast<int>(s.sold) << ' ' << s.price << ' ';
        witem(f, s.item);
    }

    // Quests
    f << g.enemiesKilled_ << ' ' << g.chestsOpened_ << '\n';
    f << g.quests_.size() << '\n';
    for (const auto& q : g.quests_) {
        f << static_cast<int>(q.status) << ' ' << q.objectives.size();
        for (const auto& obj : q.objectives)
            f << ' ' << static_cast<int>(obj.completed);
        f << '\n';
    }

    // HUD layout
    f << static_cast<int>(g.hudLayout_) << '\n';
}

// ─── load ─────────────────────────────────────────────────────────────────────

bool GameSerializer::load(Game& g)
{
    std::ifstream f(savePath());
    if (!f) return false;

    int version;
    if (!(f >> version) || version != 1)
        return false;

    // Player
    std::string name;
    if (!rstr(f, name)) return false;
    int cls;
    if (!(f >> cls)) return false;

    int level, xp, xpNext, hp, maxHp, mana, maxMana, coins, floor,
        atk, def, baseAtk, baseDef;
    if (!(f >> level >> xp >> xpNext >> hp >> maxHp >> mana >> maxMana
            >> coins >> floor >> atk >> def >> baseAtk >> baseDef))
        return false;

    PlayerClass playerClass = static_cast<PlayerClass>(cls);
    g.player_ = std::make_unique<Player>(name, playerClass);
    g.player_->level_          = level;
    g.player_->xp_             = xp;
    g.player_->xpToNextLevel_  = xpNext;
    g.player_->hp_             = hp;
    g.player_->maxHp_          = maxHp;
    g.player_->mana_           = mana;
    g.player_->maxMana_        = maxMana;
    g.player_->coins_          = coins;
    g.player_->dungeonFloor_   = floor;
    g.player_->attack_         = atk;
    g.player_->defense_        = def;
    g.player_->baseAttack_     = baseAtk;
    g.player_->baseDefense_    = baseDef;

    int hasWeapon;
    if (!(f >> hasWeapon)) return false;
    if (hasWeapon) {
        Item w;
        if (!ritem(f, w)) return false;
        g.player_->equippedWeapon_ = w;
    }

    int hasArmor;
    if (!(f >> hasArmor)) return false;
    if (hasArmor) {
        Item a;
        if (!ritem(f, a)) return false;
        g.player_->equippedArmor_ = a;
    }

    size_t invCount;
    if (!(f >> invCount)) return false;
    for (size_t i = 0; i < invCount; i++) {
        Item item;
        if (!ritem(f, item)) return false;
        g.player_->getInventory().addItem(item);
    }

    // Map
    int mapW, mapH, px, py;
    if (!(f >> mapW >> mapH >> px >> py)) return false;
    g.map_ = std::make_unique<Map>(mapW, mapH);
    g.map_->setPlayerPos(px, py);
    for (int y = 0; y < mapH; y++) {
        for (int x = 0; x < mapW; x++) {
            int type, explored, visible;
            if (!(f >> type >> explored >> visible)) return false;
            Tile& t = g.map_->at(x, y);
            t.type     = static_cast<TileType>(type);
            t.glyph    = glyphForTile(t.type);
            t.explored = explored;
            t.visible  = visible;
        }
    }
    g.map_->updateFov();

    // World enemies
    size_t enemyCount;
    if (!(f >> enemyCount)) return false;
    g.worldEnemies_.resize(enemyCount);
    for (auto& we : g.worldEnemies_) {
        int type, alive, boss;
        if (!(f >> we.pos.x >> we.pos.y >> we.spawnPos.x >> we.spawnPos.y
                >> type >> alive >> boss))
            return false;
        we.type   = static_cast<EnemyType>(type);
        we.alive  = alive;
        we.isBoss = boss;
    }

    // World chests
    size_t chestCount;
    if (!(f >> chestCount)) return false;
    g.worldChests_.resize(chestCount);
    for (auto& ch : g.worldChests_) {
        int opened, loot;
        if (!(f >> ch.pos.x >> ch.pos.y >> opened >> loot >> ch.coins))
            return false;
        ch.opened = opened;
        ch.loot   = static_cast<ChestLoot>(loot);
        if (!ritem(f, ch.item)) return false;
    }

    // Locked door
    int ldExists;
    if (!(f >> ldExists)) return false;
    g.lockedDoorExists_ = ldExists;
    g.lockedDoorOpen_   = false;
    if (g.lockedDoorExists_) {
        int ldOpen;
        if (!(f >> g.lockedDoorPos_.x >> g.lockedDoorPos_.y >> ldOpen))
            return false;
        g.lockedDoorOpen_ = ldOpen;
    }

    // Stairs
    if (!(f >> g.stairsPos_.x >> g.stairsPos_.y)) return false;

    // Shop
    int shopEx;
    if (!(f >> shopEx)) return false;
    g.shopExists_ = shopEx;
    if (g.shopExists_)
        if (!(f >> g.shopMerchantPos_.x >> g.shopMerchantPos_.y
                >> g.shopRoom_.x >> g.shopRoom_.y
                >> g.shopRoom_.w >> g.shopRoom_.h))
            return false;
    size_t shopCount;
    if (!(f >> shopCount)) return false;
    g.shopStock_.resize(shopCount);
    for (auto& s : g.shopStock_) {
        int sold;
        if (!(f >> sold >> s.price)) return false;
        s.sold = sold;
        if (!ritem(f, s.item)) return false;
    }

    // Quests
    if (!(f >> g.enemiesKilled_ >> g.chestsOpened_)) return false;
    size_t questCount;
    if (!(f >> questCount)) return false;
    g.initQuests();
    for (size_t i = 0; i < std::min(questCount, g.quests_.size()); i++) {
        int status, objCount;
        if (!(f >> status >> objCount)) return false;
        g.quests_[i].status = static_cast<QuestStatus>(status);
        for (int j = 0; j < objCount && j < static_cast<int>(g.quests_[i].objectives.size()); j++) {
            int completed;
            if (!(f >> completed)) return false;
            g.quests_[i].objectives[j].completed = completed;
        }
    }

    // HUD
    int hud;
    if (!(f >> hud)) return false;
    g.hudLayout_ = static_cast<HudLayout>(hud);

    // Restore runtime state
    g.combat_.reset();
    g.combatWorldEnemyIdx_ = -1;
    g.pendingCombatEnemy_  = -1;
    g.explorationMsg_.clear();
    g.state_ = GameState::Exploration;

    return true;
}
