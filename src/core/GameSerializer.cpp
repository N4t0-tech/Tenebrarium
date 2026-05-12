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

void GameSerializer::deleteSave()
{
    std::filesystem::remove(savePath());
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
      << item.slots << ' ' << item.statBonus << ' ' << item.quantity << '\n';
}

bool GameSerializer::ritem(std::istream& in, Item& item, int version)
{
    if (!rstr(in, item.name) || !rstr(in, item.description))
        return false;
    int t;
    if (!(in >> t >> item.value >> item.slots >> item.statBonus))
        return false;
    item.type = static_cast<ItemType>(t);
    if (version >= 2) {
        if (!(in >> item.quantity))
            return false;
    } else {
        item.quantity = 1; // Default for version 1
    }
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
    if (!f || !g.player_ || !g.dungeon_)
        return;

    auto acc = g.dungeon_->lock();

    f << 3 << '\n'; // version 3 adds ch.isMimic

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
    f << acc.map().width() << ' ' << acc.map().height() << '\n';
    Position pp = acc.playerPos();
    f << pp.x << ' ' << pp.y << '\n';
    for (int y = 0; y < acc.map().height(); y++) {
        for (int x = 0; x < acc.map().width(); x++) {
            const Tile& t = acc.map().at(x, y);
            f << static_cast<int>(t.type) << ' '
              << static_cast<int>(t.explored) << ' '
              << static_cast<int>(t.visible);
            f << (x < acc.map().width() - 1 ? ' ' : '\n');
        }
    }

    // World enemies
    f << acc.enemies().size() << '\n';
    for (const auto& we : acc.enemies())
        f << we.pos.x << ' ' << we.pos.y << ' '
          << we.spawnPos.x << ' ' << we.spawnPos.y << ' '
          << static_cast<int>(we.type) << ' '
          << static_cast<int>(we.alive) << ' '
          << static_cast<int>(we.isBoss) << '\n';

    // World chests
    f << acc.chests().size() << '\n';
    for (const auto& ch : acc.chests()) {
        f << ch.pos.x << ' ' << ch.pos.y << ' '
          << static_cast<int>(ch.opened) << ' '
          << static_cast<int>(ch.isMimic) << ' '
          << static_cast<int>(ch.loot) << ' '
          << ch.coins << ' ';
        witem(f, ch.item);
    }

    // Locked door
    f << static_cast<int>(acc.lockedDoorExists()) << ' ';
    if (acc.lockedDoorExists())
        f << acc.lockedDoorPos().x << ' ' << acc.lockedDoorPos().y << ' '
          << static_cast<int>(acc.lockedDoorOpen());
    f << '\n';

    // Stairs
    f << acc.stairsPos().x << ' ' << acc.stairsPos().y << '\n';

    // Shop
    f << static_cast<int>(acc.shopExists()) << '\n';
    if (acc.shopExists())
        f << acc.shopMerchantPos().x << ' ' << acc.shopMerchantPos().y << ' '
          << acc.shopRoom().x << ' ' << acc.shopRoom().y << ' '
          << acc.shopRoom().w << ' ' << acc.shopRoom().h << '\n';
    f << g.shopStock_.size() << '\n';
    for (const auto& s : g.shopStock_) {
        f << static_cast<int>(s.sold) << ' ' << s.price << ' ';
        witem(f, s.item);
    }

    // Quests
    f << g.dungeon_->enemiesKilled << ' ' << g.dungeon_->chestsOpened << '\n';
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
    if (!(f >> version) || (version != 1 && version != 2 && version != 3))
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
        if (!ritem(f, w, version)) return false;
        g.player_->equippedWeapon_ = w;
    }

    int hasArmor;
    if (!(f >> hasArmor)) return false;
    if (hasArmor) {
        Item a;
        if (!ritem(f, a, version)) return false;
        g.player_->equippedArmor_ = a;
    }

    size_t invCount;
    if (!(f >> invCount)) return false;
    for (size_t i = 0; i < invCount; i++) {
        Item item;
        if (!ritem(f, item, version)) return false;
        g.player_->getInventory().addItem(item);
    }

    // Recreate dungeon
    g.dungeon_ = std::make_unique<Dungeon>();

    // Map
    int mapW, mapH, px, py;
    if (!(f >> mapW >> mapH >> px >> py)) return false;

    {
        auto acc = g.dungeon_->lock();
        auto& map = acc.map();
        map = Map(mapW, mapH);
        acc.setPlayerPos(px, py);
        for (int y = 0; y < mapH; y++) {
            for (int x = 0; x < mapW; x++) {
                int type, explored, visible;
                if (!(f >> type >> explored >> visible)) return false;
                Tile& t = map.at(x, y);
                t.type     = static_cast<TileType>(type);
                t.glyph    = glyphForTile(t.type);
                t.explored = explored;
                t.visible  = visible;
            }
        }
        map.updateFov();
    }

    // World enemies
    size_t enemyCount;
    if (!(f >> enemyCount)) return false;
    {
        auto acc = g.dungeon_->lock();
        auto& enemies = acc.enemies();
        enemies.resize(enemyCount);
        for (auto& we : enemies) {
            int type, alive, boss;
            if (!(f >> we.pos.x >> we.pos.y >> we.spawnPos.x >> we.spawnPos.y
                    >> type >> alive >> boss))
                return false;
            we.type   = static_cast<EnemyType>(type);
            we.alive  = alive;
            we.isBoss = boss;
        }
    }

    // World chests
    size_t chestCount;
    if (!(f >> chestCount)) return false;
    {
        auto acc = g.dungeon_->lock();
        auto& chests = acc.chests();
        chests.resize(chestCount);
        for (auto& ch : chests) {
            int opened, loot;
            if (!(f >> ch.pos.x >> ch.pos.y >> opened))
                return false;
            ch.opened = opened;
            if (version >= 3) {
                int mimic;
                if (!(f >> mimic)) return false;
                ch.isMimic = mimic != 0;
            }
            if (!(f >> loot >> ch.coins)) return false;
            ch.loot   = static_cast<ChestLoot>(loot);
            if (!ritem(f, ch.item, version)) return false;
        }
    }

    // Locked door
    int ldExists;
    if (!(f >> ldExists)) return false;
    {
        auto acc = g.dungeon_->lock();
        acc.self.lockedDoorExists_ = ldExists;
        acc.self.lockedDoorOpen_   = false;
        if (acc.lockedDoorExists()) {
            int ldOpen;
            if (!(f >> acc.self.lockedDoorPos_.x >> acc.self.lockedDoorPos_.y >> ldOpen))
                return false;
            acc.self.lockedDoorOpen_ = ldOpen;
        }
    }

    // Stairs
    Position stairsPos;
    if (!(f >> stairsPos.x >> stairsPos.y)) return false;
    {
        auto acc = g.dungeon_->lock();
        acc.self.stairsPos_ = stairsPos;
    }

    // Shop
    int shopEx;
    if (!(f >> shopEx)) return false;
    {
        auto acc = g.dungeon_->lock();
        acc.self.shopExists_ = shopEx;
        if (acc.shopExists())
            if (!(f >> acc.self.shopMerchantPos_.x >> acc.self.shopMerchantPos_.y
                    >> acc.self.shopRoom_.x >> acc.self.shopRoom_.y
                    >> acc.self.shopRoom_.w >> acc.self.shopRoom_.h))
                return false;
    }
    size_t shopCount;
    if (!(f >> shopCount)) return false;
    g.shopStock_.resize(shopCount);
    for (auto& s : g.shopStock_) {
        int sold;
        if (!(f >> sold >> s.price)) return false;
        s.sold = sold;
        if (!ritem(f, s.item, version)) return false;
    }

    // Quests
    int savedKilled = 0, savedChests = 0;
    if (!(f >> savedKilled >> savedChests)) return false;
    g.dungeon_->enemiesKilled = savedKilled;
    g.dungeon_->chestsOpened  = savedChests;
    size_t questCount;
    if (!(f >> questCount)) return false;
    g.initQuests();
    g.dungeon_->enemiesKilled = savedKilled;
    g.dungeon_->chestsOpened  = savedChests;
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
    g.dungeon_->message.clear();
    g.state_.store(GameState::Exploration);

    return true;
}
