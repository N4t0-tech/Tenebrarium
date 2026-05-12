#include "DungeonPopulator.hpp"
#include <algorithm>
#include <cstdlib>

// ─── Enemy tables ─────────────────────────────────────────────────────────────

// pickEnemyType — selección aleatoria ponderada de tipo de enemigo según el piso.
// Zombies desde piso 2, Sombras desde piso 4, Demonios desde piso 6.
EnemyType DungeonPopulator::pickEnemyType(int floor)
{
    if (floor == 1) return EnemyType::Slime;

    bool zombieAvail  = (floor >= 2);
    bool shadowAvail  = (floor >= 4);
    bool demonAvail   = (floor >= 6);
    int gobW = std::max(5, 55 - (floor - 1) * 8);
    int skeW = 15;
    int orcW = std::min(30, (floor - 1) * 5);
    int spiW = std::min(30, (floor - 2) * 7);
    int vapW = std::min(20, (floor - 4) * 5);
    int zomW = zombieAvail  ? std::min(25, (floor - 2) * 6) : 0;
    int shaW = shadowAvail  ? std::min(20, (floor - 4) * 5) : 0;
    int demW = demonAvail   ? std::min(20, (floor - 6) * 5) : 0;
    int total = gobW + skeW + orcW + spiW + vapW + zomW + shaW + demW;
    int r = std::rand() % total;
    if (r < gobW) return EnemyType::Goblin;
    r -= gobW;
    if (r < skeW) return EnemyType::Skeleton;
    r -= skeW;
    if (r < orcW) return EnemyType::Orc;
    r -= orcW;
    if (r < spiW) return EnemyType::Spider;
    r -= spiW;
    if (r < vapW) return EnemyType::Vampire;
    r -= vapW;
    if (r < zomW) return EnemyType::Zombie;
    r -= zomW;
    if (r < shaW) return EnemyType::Shadow;
    return EnemyType::Demon;
}

std::unique_ptr<Enemy> DungeonPopulator::makeEnemy(EnemyType t, int floor, bool isBoss)
{
    float s  = 1.0f + (floor - 1) * 0.15f;
    auto  sc = [s](int base) { return std::max(1, static_cast<int>(base * s)); };
    std::string name;
    int hp, atk, def, xp, pa = 1;
    switch (t) {
    case EnemyType::Goblin:
        name = "Goblin";   hp = sc(125); atk = sc(17); def = sc(4);  xp = sc(60);  pa = 1; break;
    case EnemyType::Skeleton:
        name = "Esqueleto"; hp = sc(120); atk = sc(16); def = sc(8);  xp = sc(60);  pa = 1; break;
    case EnemyType::Orc:
        name = "Orco";     hp = sc(140); atk = sc(20); def = sc(10); xp = sc(120); pa = 2; break;
    case EnemyType::Spider:
        name = "Araña";    hp = sc(80);  atk = sc(12); def = sc(2);  xp = sc(40);  pa = 1; break;
    case EnemyType::Vampire:
        name = "Vampiro";  hp = sc(110); atk = sc(18); def = sc(6);  xp = sc(80);  pa = 1; break;
    case EnemyType::Zombie:
        name = "Zombie";   hp = sc(160); atk = sc(10); def = sc(15); xp = sc(50);  pa = 1; break;
    case EnemyType::Demon:
        name = "Demonio";  hp = sc(150); atk = sc(25); def = sc(8);  xp = sc(100); pa = 1; break;
    case EnemyType::Shadow:
        name = "Sombra";   hp = sc(70);  atk = sc(18); def = sc(2);  xp = sc(35);  pa = 2; break;
    case EnemyType::Mimic:
        name = "Mímico";   hp = sc(110); atk = sc(18); def = sc(10); xp = sc(30);  pa = 1; break;
    case EnemyType::Slime:
        name = "Slime";    hp = sc(60);  atk = sc(8);  def = sc(1);  xp = sc(10);  pa = 1; break;
    default:
        name = "???";      hp = 10;      atk = 5;      def = 1;      xp = 5;       pa = 1; break;
    }
    if (isBoss) {
        hp  = static_cast<int>(hp  * 2.5f);
        atk = static_cast<int>(atk * 2.5f);
        def = static_cast<int>(def * 2.5f);
        switch (t) {
        case EnemyType::Goblin:   name = "Rey Goblin";      break;
        case EnemyType::Skeleton: name = "Señor Liche";     break;
        case EnemyType::Orc:      name = "Gran Orco";       break;
        case EnemyType::Spider:   name = "Reina Araña";     break;
        case EnemyType::Vampire:  name = "Vampiro Anciano"; break;
        case EnemyType::Zombie:   name = "Zombie Colosal";  break;
        case EnemyType::Demon:    name = "Archidemonio";    break;
        case EnemyType::Shadow:   name = "Sombra Eterna";   break;
        default: break;
        }
    }
    auto enemy = std::make_unique<Enemy>(name, hp, atk, def, xp, t, pa);
    if (t == EnemyType::Slime && !isBoss)
        enemy->setSplitsOnDeath(true);
    return enemy;
}

int DungeonPopulator::xpForEnemy(EnemyType t, int floor)
{
    int base;
    switch (t) {
    case EnemyType::Goblin:   base = 20; break;
    case EnemyType::Skeleton: base = 15; break;
    case EnemyType::Orc:      base = 30; break;
    case EnemyType::Spider:   base = 40; break;
    case EnemyType::Vampire:  base = 80; break;
    case EnemyType::Zombie:   base = 25; break;
    case EnemyType::Demon:    base = 60; break;
    case EnemyType::Shadow:   base = 18; break;
    case EnemyType::Mimic:    base = 25; break;
    case EnemyType::Slime:    base =  5; break;
    default:                  base =  5; break;
    }
    return base + (floor - 1) * 5;
}

// ─── Item tables ──────────────────────────────────────────────────────────────

Item DungeonPopulator::pickWeapon(PlayerClass cls, int floor)
{
    static const std::vector<Item> warrior = {
        {"Espada Corta",   "Un filo confiable.",       ItemType::Weapon, 30, 1, 3, 1},
        {"Estoque",        "Rápido y preciso.",         ItemType::Weapon, 30, 1, 3, 1},
        {"Hacha de Mano",  "Golpea con fuerza.",        ItemType::Weapon, 50, 1, 5, 1},
        {"Espada",         "Equilibrada y versátil.",   ItemType::Weapon, 50, 1, 5, 1},
        {"Mandoble",       "Lenta pero letal.",         ItemType::Weapon, 80, 2, 8, 1},
        {"Lanza",          "Mantiene la distancia.",    ItemType::Weapon, 80, 2, 7, 1},
    };
    static const std::vector<Item> mage = {
        {"Varita de Roble",   "Canaliza magia.",           ItemType::Weapon, 30, 1, 2, 1},
        {"Varita de Sauco",   "Madera antigua y potente.", ItemType::Weapon, 30, 1, 2, 1},
        {"Baculo de Cristal", "Poder arcano.",             ItemType::Weapon, 50, 1, 4, 1},
        {"Baculo de Rayos",   "Conduce electricidad.",     ItemType::Weapon, 50, 1, 4, 1},
        {"Grimorio Oscuro",   "Magia devastadora.",        ItemType::Weapon, 80, 2, 6, 1},
        {"Grimorio Demoniaco","Conocimiento prohibido.",   ItemType::Weapon, 80, 2, 7, 1},
    };
    static const std::vector<Item> ranger = {
        {"Arco Corto",     "Rápido y preciso.",         ItemType::Weapon, 30, 1, 3, 1},
        {"Honda",          "Simple pero efectiva.",     ItemType::Weapon, 30, 1, 2, 1},
        {"Arco Largo",     "Mayor alcance.",            ItemType::Weapon, 50, 1, 5, 1},
        {"Arco Elfico",    "Tallado en madera élfica.", ItemType::Weapon, 50, 1, 5, 1},
        {"Ballesta",       "Poderosa y lenta.",         ItemType::Weapon, 80, 2, 7, 1},
        {"Arco Encantado", "Flechas mágicas.",          ItemType::Weapon, 80, 2, 7, 1},
        {"Arco Celestial", "Bendecido por los dioses.", ItemType::Weapon, 80, 2, 8, 1},
    };
    const auto& pool = (cls == PlayerClass::Warrior) ? warrior
                     : (cls == PlayerClass::Mage)    ? mage
                                                     : ranger;
    Item item = pool[std::rand() % pool.size()];
    item.statBonus += (floor - 1) / 2;
    return item;
}

Item DungeonPopulator::pickArmor(PlayerClass cls, int floor)
{
    static const Item a[3][2] = {
        {{"Cota de Malla",      "Protección media.",   ItemType::Armor, 40, 1, 3, 1},
         {"Armadura de Placas", "Muy resistente.",     ItemType::Armor, 70, 2, 6, 1}},
         {{"Túnica Arcana",      "Ligera y mágica.",    ItemType::Armor, 30, 1, 1, 1},
         {"Manto Mistico",      "Deflecta hechizos.",  ItemType::Armor, 60, 1, 3, 1}},
        {{"Cuero Reforzado",    "Ágil y resistente.",  ItemType::Armor, 35, 1, 2, 1},
         {"Armadura de Cuero",  "Balance perfecto.",   ItemType::Armor, 55, 1, 4, 1}},
    };
    int ci = (cls == PlayerClass::Warrior) ? 0 : (cls == PlayerClass::Mage) ? 1 : 2;
    Item item = a[ci][std::rand() % 2];
    item.statBonus += (floor - 1) / 2;
    return item;
}

Item DungeonPopulator::pickPotion(int floor)
{
    if (floor >= 5 || std::rand() % 3 == 0)
        return {"Poción Mayor",    "Recupera 80 HP.", ItemType::Consumable, 50, 1, 80, 1};
    if (floor >= 3 || std::rand() % 2 == 0)
        return {"Poción de Vida",  "Recupera 40 HP.", ItemType::Consumable, 25, 1, 40, 1};
    return     {"Poción Pequeña",  "Recupera 20 HP.", ItemType::Consumable, 12, 1, 20, 1};
}

Item DungeonPopulator::pickBeer()
{
    return {"Cerveza", "Restaura la mitad del aguante.", ItemType::Consumable, 5, 1, 0, 1};
}

Item DungeonPopulator::pickManaPotion()
{
    return {"Poción de Mana", "Restaura la mitad del mana.", ItemType::Consumable, 8, 1, 0, 1};
}

Item DungeonPopulator::pickBomb(int floor)
{
    int price = 20 + floor * 3;
    return {"Bomba", "Destruye paredes secretas.", ItemType::Bomb, price, 1, 0, 1};
}

// ─── Position helper ──────────────────────────────────────────────────────────

Position DungeonPopulator::pickPos(const BSPDungeon::Room& r, std::vector<Position>& taken)
{
    int iw = std::max(1, r.w - 2);
    int ih = std::max(1, r.h - 2);
    for (int attempt = 0; attempt < 30; attempt++) {
        Position p = {r.x + 1 + std::rand() % iw,
                      r.y + 1 + std::rand() % ih};
        bool ok = true;
        for (const auto& t : taken)
            if (t.x == p.x && t.y == p.y) { ok = false; break; }
        if (ok) { taken.push_back(p); return p; }
    }
    Position c = {r.centerX(), r.centerY()};
    taken.push_back(c);
    return c;
}

// ─── Secret rooms ─────────────────────────────────────────────────────────────

void DungeonPopulator::tryPlaceSecretRoom(Map& map, std::vector<WorldChest>& chests,
                                          std::vector<Position>& taken,
                                          PlayerClass cls, int floor)
{
    static const int DIRS[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    static const int INNER = 3;

    struct Candidate { int wallX, wallY, dx, dy; };
    std::vector<Candidate> cands;

    for (int y = 2; y < map.height() - 2; y++) {
        for (int x = 2; x < map.width() - 2; x++) {
            if (map.at(x, y).type != TileType::Floor)
                continue;
            for (auto& d : DIRS) {
                int wx = x + d[0], wy = y + d[1];
                if (wx < 1 || wx >= map.width() - 1 ||
                    wy < 1 || wy >= map.height() - 1)
                    continue;
                if (map.at(wx, wy).type == TileType::Wall)
                    cands.push_back({wx, wy, d[0], d[1]});
            }
        }
    }

    for (int i = static_cast<int>(cands.size()) - 1; i > 0; i--)
        std::swap(cands[i], cands[std::rand() % (i + 1)]);

    for (const auto& c : cands) {
        int startX, startY, endX, endY;
        if (c.dx != 0) {
            startX = c.wallX + (c.dx > 0 ? 1 : -INNER);
            endX   = c.wallX + (c.dx > 0 ? INNER : -1);
            startY = c.wallY - INNER / 2;
            endY   = c.wallY + INNER / 2;
        } else {
            startX = c.wallX - INNER / 2;
            endX   = c.wallX + INNER / 2;
            startY = c.wallY + (c.dy > 0 ? 1 : -INNER);
            endY   = c.wallY + (c.dy > 0 ? INNER : -1);
        }

        if (startX < 1 || endX >= map.width() - 1 ||
            startY < 1 || endY >= map.height() - 1)
            continue;

        bool valid = true;
        for (int by = startY - 1; by <= endY + 1 && valid; by++)
            for (int bx = startX - 1; bx <= endX + 1 && valid; bx++) {
                if (bx == c.wallX && by == c.wallY) continue;
                if (map.at(bx, by).type != TileType::Wall) valid = false;
            }
        if (!valid) continue;

        for (int y = startY; y <= endY; y++)
            for (int x = startX; x <= endX; x++)
                map.at(x, y) = {TileType::Floor, '.', false, false};
        map.at(c.wallX, c.wallY).type = TileType::SecretWall;

        Position itemPos  = {startX + 1, startY + 1};
        Position coinsPos = {endX   - 1, endY   - 1};
        taken.push_back(itemPos);
        Item item = (std::rand() % 2 == 0) ? pickWeapon(cls, floor) : pickArmor(cls, floor);
        chests.push_back({itemPos, false, ChestLoot::Item, 0, item});

        if (coinsPos.x != itemPos.x || coinsPos.y != itemPos.y) {
            taken.push_back(coinsPos);
            if (std::rand() % 10 < 3)
                chests.push_back({coinsPos, false, ChestLoot::Item, 0, pickPotion(floor)});
            else {
                int coins = 20 + std::rand() % 31;
                chests.push_back({coinsPos, false, ChestLoot::Coins, coins, {}});
            }
        }
        return;
    }
}

// ─── Main population ──────────────────────────────────────────────────────────

// populate() — asigna contenido a las salas generadas por BSPDungeon.
// Asignación de salas por rol (el índice 0 = sala inicial del jugador):
//   rooms[0]       → sala de spawn del jugador (reservada, no se puebla)
//   rooms[n-1]     → escaleras al siguiente piso
//   rooms[n-2]     → puerta bloqueada (requiere matar todos los enemigos)
//   rooms[i]       → tienda (una sala aleatoria entre inicio y fin)
//   resto          → enemigos, cofres, salas secretas
// El vector 'taken' acumula posiciones ya ocupadas para que pickPos() las evite.
DungeonPopulator::Result DungeonPopulator::populate(
    Map& map, const std::vector<BSPDungeon::Room>& rooms, int floor, PlayerClass cls)
{
    Result result;
    int n = static_cast<int>(rooms.size());

    std::vector<Position> taken;
    taken.push_back({rooms[0].centerX(), rooms[0].centerY()});  // posición inicial del jugador


    if (n >= 3) {
        result.lockedDoorPos    = pickPos(rooms[n - 2], taken);
        result.lockedDoorExists = true;
        result.stairsPos        = result.lockedDoorPos;
    } else {
        // Stairs in last room when there is no locked door
        result.stairsPos = pickPos(rooms[n - 1], taken);
    }

    // Tienda: cualquier sala que no sea inicio, fin ni sala de puerta bloqueada
    {
        std::vector<int> shopCandidates;
        for (int i = 1; i < n - 1; i++) {
            if (n >= 3 && i == n - 2) continue;
            shopCandidates.push_back(i);
        }
        if (!shopCandidates.empty()) {
            int si = shopCandidates[std::rand() % static_cast<int>(shopCandidates.size())];
            result.shopRoom        = rooms[si];
            result.shopMerchantPos = {result.shopRoom.centerX(), result.shopRoom.centerY()};
            result.shopExists      = true;
            taken.push_back(result.shopMerchantPos);
        }
    }

    // spawnChance crece con el piso (más difícil = más enemigos por sala)
    int spawnChance = std::min(85, 50 + (floor - 1) * 8);
    for (int i = 1; i < n; i++) {
        if (result.shopExists &&
            rooms[i].x == result.shopRoom.x && rooms[i].y == result.shopRoom.y)
            continue;
        if (std::rand() % 100 < spawnChance)
            result.enemies.push_back({pickPos(rooms[i], taken), pickPos(rooms[i], taken),
                                      pickEnemyType(floor), true});
        if (floor >= 3 && std::rand() % 100 < 20 + floor * 3)
            result.enemies.push_back({pickPos(rooms[i], taken), pickPos(rooms[i], taken),
                                      pickEnemyType(floor), true});
    }

    // Chests
    int chestCount = 2 + std::rand() % 3;
    std::vector<int> eligible;
    for (int i = 1; i < n - 1; i++) eligible.push_back(i);
    for (int i = static_cast<int>(eligible.size()) - 1; i > 0; i--)
        std::swap(eligible[i], eligible[std::rand() % (i + 1)]);
    chestCount = std::min(chestCount, static_cast<int>(eligible.size()));

    for (int ci = 0; ci < chestCount; ci++) {
        int roll = std::rand() % 100;
        ChestLoot loot;
        int coins = 0;
        Item item{};
        int baseCoins = 10 + std::rand() % 40;
        if (roll < 55) {
            loot  = ChestLoot::Coins;
            coins = baseCoins * (1 + (floor - 1) / 2);
        } else {
            loot     = ChestLoot::Item;
            int r    = std::rand() % 4;
            if      (r == 0) item = pickPotion(floor);
            else if (r == 1) item = pickWeapon(cls, floor);
            else             item = pickArmor(cls, floor);
        }
        result.chests.push_back({pickPos(rooms[eligible[ci]], taken), false, loot, coins, item});
    }

    // Mimics: small chance (~12%) that a chest is actually a mimic
    for (auto& ch : result.chests)
        if (!ch.opened && std::rand() % 100 < 12)
            ch.isMimic = true;

    // Secret rooms
    int numSecret = 1 + std::rand() % 2;
    for (int i = 0; i < numSecret; i++)
        tryPlaceSecretRoom(map, result.chests, taken, cls, floor);

    // Boss every 5 floors
    if (floor % 5 == 0 && floor > 0 && n >= 3)
        result.enemies.push_back({pickPos(rooms[n - 2], taken), pickPos(rooms[n - 2], taken),
                                  pickEnemyType(floor), true, true});

    return result;
}
