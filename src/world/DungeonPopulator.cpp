#include "DungeonPopulator.hpp"
#include <algorithm>
#include <random>

// ─── RNG helper ───────────────────────────────────────────────────────────────

static int rngInt(std::mt19937& rng, int lo, int hi) {
    if (lo >= hi) return lo;
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

// ─── Enemy tables ─────────────────────────────────────────────────────────────

EnemyType DungeonPopulator::pickEnemyType(int floor, std::mt19937& rng)
{
    bool spiderAvail  = (floor >= 2);
    bool zombieAvail  = (floor >= 2);
    bool vampireAvail = (floor >= 4);
    bool shadowAvail  = (floor >= 4);
    bool demonAvail   = (floor >= 6);
    int sliW = std::max(0, 20 - (floor - 1) * 3);
    int gobW = std::max(5, 55 - (floor - 1) * 8);
    int skeW = 15;
    int orcW = std::min(30, (floor - 1) * 5);
    int spiW = spiderAvail  ? std::min(30, (floor - 2) * 7) : 0;
    int vapW = vampireAvail ? std::min(20, (floor - 4) * 5) : 0;
    int zomW = zombieAvail  ? std::min(25, (floor - 2) * 6) : 0;
    int shaW = shadowAvail  ? std::min(20, (floor - 4) * 5) : 0;
    int demW = demonAvail   ? std::min(20, (floor - 6) * 5) : 0;
    int total = sliW + gobW + skeW + orcW + spiW + vapW + zomW + shaW + demW;
    int r = rngInt(rng, 0, total - 1);
    if (r < sliW) return EnemyType::Slime;
    r -= sliW;
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

float DungeonPopulator::difficultyFactor(int depth, int incursions)
{
    return (1.0f + (depth - 1) * 0.09f) * (1.0f + (incursions - 1) * 0.10f);
}

std::unique_ptr<Enemy> DungeonPopulator::makeEnemy(EnemyType t, int depth, int incursions, bool isBoss)
{
    float s  = difficultyFactor(depth, incursions);
    auto  sc = [s](int base) { return std::max(1, static_cast<int>(base * s)); };
    std::string name;
    int hp, atk, def, xp, pa = 1;
    switch (t) {
    case EnemyType::Goblin:
        name = "Goblin";   hp = sc(125); atk = sc(17); def = sc(4);  xp = sc(60);  pa = 1; break;
    case EnemyType::Skeleton:
        name = "Esqueleto"; hp = sc(120); atk = sc(16); def = sc(4);  xp = sc(60);  pa = 1; break;
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

int DungeonPopulator::xpForEnemy(EnemyType t, int depth, int incursions)
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
    return static_cast<int>((base + (depth - 1) * 5) * difficultyFactor(depth, incursions));
}

// ─── Item tables ──────────────────────────────────────────────────────────────

namespace {

struct GearEntry {
    PlayerClass cls;
    Item        item;
};

// Precio de referencia por tier: Común 25–40, Raro 60–90, Épico 140–200, Legendario 350+.
const std::vector<GearEntry>& gearTable()
{
    using PC = PlayerClass;
    static const std::vector<GearEntry> t = {
        // ─── Guerrero: armas ───
        {PC::Warrior, {"Espada Corta",        "Un filo confiable.",                ItemType::Weapon, 30, 1,  3, 1, 1}},
        {PC::Warrior, {"Estoque",             "Rápido y preciso.",                 ItemType::Weapon, 30, 1,  3, 1, 1}},
        {PC::Warrior, {"Maza de Hierro",      "Abolla yelmos y huesos.",           ItemType::Weapon, 40, 1,  4, 1, 1}},
        {PC::Warrior, {"Hacha de Mano",       "Golpea con fuerza.",                ItemType::Weapon, 60, 1,  5, 1, 2}},
        {PC::Warrior, {"Espada",              "Equilibrada y versátil.",           ItemType::Weapon, 70, 1,  6, 1, 2}},
        {PC::Warrior, {"Martillo de Guerra",  "Cada golpe retumba.",               ItemType::Weapon, 90, 2,  7, 1, 2}},
        {PC::Warrior, {"Lanza",               "Mantiene la distancia.",            ItemType::Weapon, 150, 2, 9, 1, 3}},
        {PC::Warrior, {"Mandoble",            "Lenta pero letal.",                 ItemType::Weapon, 170, 2, 10, 1, 3}},
        {PC::Warrior, {"Hacha del Verdugo",   "Nunca necesitó un segundo tajo.",   ItemType::Weapon, 200, 2, 11, 1, 3}},
        {PC::Warrior, {"Quebrantamontes",     "Martillo de los reyes enanos.",     ItemType::Weapon, 380, 2, 15, 1, 4}},
        // ─── Guerrero: armaduras ───
        {PC::Warrior, {"Gambesón",            "Tela acolchada, mejor que nada.",   ItemType::Armor, 25, 1,  2, 1, 1}},
        {PC::Warrior, {"Cota de Malla",       "Protección media.",                 ItemType::Armor, 40, 1,  3, 1, 1}},
        {PC::Warrior, {"Brigantina",          "Placas remachadas bajo el cuero.",  ItemType::Armor, 65, 1,  4, 1, 2}},
        {PC::Warrior, {"Escudo de Roble",     "Pesado, pero aguanta.",             ItemType::Armor, 85, 1,  5, 1, 2}},
        {PC::Warrior, {"Armadura de Placas",  "Muy resistente.",                   ItemType::Armor, 160, 2, 7, 1, 3}},
        {PC::Warrior, {"Placas Templarias",   "Bendecidas en el último altar.",    ItemType::Armor, 195, 2, 8, 1, 3}},
        {PC::Warrior, {"Égida del Rey Caído", "Su dueño murió de viejo.",          ItemType::Armor, 400, 2, 11, 1, 4}},
        // ─── Mago: armas ───
        {PC::Mage,    {"Varita de Roble",     "Canaliza magia.",                   ItemType::Weapon, 25, 1,  2, 1, 1}},
        {PC::Mage,    {"Varita de Saúco",     "Madera antigua y potente.",         ItemType::Weapon, 35, 1,  3, 1, 1}},
        {PC::Mage,    {"Bastón de Fresno",    "Firme como su dueño.",              ItemType::Weapon, 35, 1,  3, 1, 1}},
        {PC::Mage,    {"Báculo de Cristal",   "Poder arcano.",                     ItemType::Weapon, 60, 1,  5, 1, 2}},
        {PC::Mage,    {"Báculo de Rayos",     "Conduce electricidad.",             ItemType::Weapon, 80, 1,  6, 1, 2}},
        {PC::Mage,    {"Orbe de Cuarzo",      "Vibra con cada conjuro.",           ItemType::Weapon, 80, 1,  6, 1, 2}},
        {PC::Mage,    {"Grimorio Oscuro",     "Magia devastadora.",                ItemType::Weapon, 150, 2, 9, 1, 3}},
        {PC::Mage,    {"Grimorio Demoníaco",  "Conocimiento prohibido.",           ItemType::Weapon, 175, 2, 10, 1, 3}},
        {PC::Mage,    {"Báculo del Archimago","Tres siglos de hechizos grabados.", ItemType::Weapon, 200, 2, 11, 1, 3}},
        {PC::Mage,    {"Lengua de Dragón",    "Aún guarda el fuego de la bestia.", ItemType::Weapon, 380, 2, 14, 1, 4}},
        // ─── Mago: armaduras ───
        {PC::Mage,    {"Túnica Arcana",       "Ligera y mágica.",                  ItemType::Armor, 25, 1,  1, 1, 1}},
        {PC::Mage,    {"Capa de Aprendiz",    "Huele a tinta y velas.",            ItemType::Armor, 30, 1,  2, 1, 1}},
        {PC::Mage,    {"Manto Místico",       "Deflecta hechizos.",                ItemType::Armor, 65, 1,  4, 1, 2}},
        {PC::Mage,    {"Toga de la Academia", "Bordada con runas de guarda.",      ItemType::Armor, 80, 1,  5, 1, 2}},
        {PC::Mage,    {"Manto de Seda Lunar", "Brilla en la oscuridad.",           ItemType::Armor, 145, 1, 6, 1, 3}},
        {PC::Mage,    {"Vestiduras del Oráculo","Ven el golpe antes que tú.",      ItemType::Armor, 180, 1, 7, 1, 3}},
        {PC::Mage,    {"Manto del Primer Hechicero","Tejido antes que las torres.",ItemType::Armor, 380, 1, 9, 1, 4}},
        // ─── Arquera: armas ───
        {PC::Ranger,  {"Honda de Cuero",      "Piedras y paciencia.",              ItemType::Weapon, 25, 1,  2, 1, 1}},
        {PC::Ranger,  {"Arco Corto",          "Rápido y preciso.",                 ItemType::Weapon, 30, 1,  3, 1, 1}},
        {PC::Ranger,  {"Arco de Caza",        "Para presas que corren.",           ItemType::Weapon, 40, 1,  4, 1, 1}},
        {PC::Ranger,  {"Arco Largo",          "Mayor alcance.",                    ItemType::Weapon, 60, 1,  5, 1, 2}},
        {PC::Ranger,  {"Arco Élfico",         "Tallado en madera élfica.",         ItemType::Weapon, 75, 1,  6, 1, 2}},
        {PC::Ranger,  {"Ballesta de Mano",    "Pequeña y traicionera.",            ItemType::Weapon, 90, 1,  7, 1, 2}},
        {PC::Ranger,  {"Ballesta",            "Poderosa y lenta.",                 ItemType::Weapon, 150, 2, 9, 1, 3}},
        {PC::Ranger,  {"Arco Encantado",      "Flechas mágicas.",                  ItemType::Weapon, 170, 2, 10, 1, 3}},
        {PC::Ranger,  {"Arco Celestial",      "Bendecido por los dioses.",         ItemType::Weapon, 200, 2, 11, 1, 3}},
        {PC::Ranger,  {"Susurro del Bosque",  "Nadie oye la flecha que lo mata.",  ItemType::Weapon, 380, 2, 15, 1, 4}},
        // ─── Arquera: armaduras ───
        {PC::Ranger,  {"Cuero Reforzado",     "Ágil y resistente.",                ItemType::Armor, 30, 1,  2, 1, 1}},
        {PC::Ranger,  {"Jubón de Cazador",    "Manchado de mil cacerías.",         ItemType::Armor, 30, 1,  2, 1, 1}},
        {PC::Ranger,  {"Armadura de Cuero",   "Balance perfecto.",                 ItemType::Armor, 65, 1,  4, 1, 2}},
        {PC::Ranger,  {"Capa de Explorador",  "Se funde con la sombra.",           ItemType::Armor, 85, 1,  5, 1, 2}},
        {PC::Ranger,  {"Cuero de Wyvern",     "Las garras resbalan.",              ItemType::Armor, 180, 1, 7, 1, 3}},
        {PC::Ranger,  {"Manto del Guardabosques","El bosque cuida de los suyos.",  ItemType::Armor, 390, 1, 10, 1, 4}},
        // ─── Halley ───
        {PC::Halley,  {"Filo del Equilibrio", "Cortes justos y precisos.",         ItemType::Weapon, 65, 1,  5, 1, 2}},
        {PC::Halley,  {"Daga Temporal",       "Ataja en el momento exacto.",       ItemType::Weapon, 80, 1,  6, 1, 2}},
        {PC::Halley,  {"Espada Eterna",       "El tiempo no la desgasta.",         ItemType::Weapon, 170, 2, 9, 1, 3}},
        {PC::Halley,  {"Guadaña del Juicio",  "Decide el destino.",                ItemType::Weapon, 380, 2, 14, 1, 4}},
        {PC::Halley,  {"Manto del Equilibrio","Pesa lo que debe pesar.",           ItemType::Armor, 80, 1,  5, 1, 2}},
        {PC::Halley,  {"Armadura Eterna",     "El tiempo la consagra.",            ItemType::Armor, 190, 2, 8, 1, 3}},
        // ─── Nato ───
        {PC::Nato,    {"Cetro Estelar",       "Brilla con luz propia.",            ItemType::Weapon, 80, 1,  6, 1, 2}},
        {PC::Nato,    {"Lanza Celestial",     "Forjada en el cosmos.",             ItemType::Weapon, 90, 1,  7, 1, 2}},
        {PC::Nato,    {"Espada del Creador",  "Fragmento de voluntad divina.",     ItemType::Weapon, 180, 2, 10, 1, 3}},
        {PC::Nato,    {"Báculo de Estrellas", "Gobierna el firmamento.",           ItemType::Weapon, 400, 2, 15, 1, 4}},
        {PC::Nato,    {"Túnica Estelar",      "Tejida con luz de estrellas.",      ItemType::Armor, 80, 1,  5, 1, 2}},
        {PC::Nato,    {"Armadura Divina",     "Voluntad hecha materia.",           ItemType::Armor, 190, 2, 8, 1, 3}},
    };
    return t;
}

// Probabilidad (%) de cada tier según el piso: Común, Raro, Épico, Legendario.
int rollTier(int floor)
{
    static const int kOdds[4][4] = {
        {70, 25,  5,  0},   // pisos 1–5
        {50, 35, 13,  2},   // pisos 6–10
        {35, 38, 22,  5},   // pisos 11–15
        {20, 40, 30, 10},   // pisos 16–20
    };
    int band = std::clamp((floor - 1) / 5, 0, 3);
    int r = std::rand() % 100;
    for (int t = 0; t < 4; t++) {
        if (r < kOdds[band][t]) return t + 1;
        r -= kOdds[band][t];
    }
    return 1;
}

Item pickGear(PlayerClass cls, ItemType type, int floor, int minTier, int maxTier)
{
    int tier = std::clamp(rollTier(floor), minTier, maxTier);
    // Si la clase no tiene ese tier (las secretas no tienen comunes, p. ej.),
    // se busca el más cercano: primero hacia abajo y luego hacia arriba.
    std::vector<const Item*> pool;
    for (int d = 0; pool.empty() && d < kMaxItemTier; d++) {
        for (int want : {tier - d, tier + d}) {
            for (const auto& g : gearTable())
                if (g.cls == cls && g.item.type == type && g.item.tier == want)
                    pool.push_back(&g.item);
            if (!pool.empty()) break;
        }
    }
    if (pool.empty())
        return {"Palo", "Mejor que nada.", type, 5, 1, 1, 1, 1};
    Item item = *pool[std::rand() % pool.size()];
    item.statBonus += (floor - 1) / 2;
    return item;
}

} // namespace

Item DungeonPopulator::pickWeapon(PlayerClass cls, int floor, int minTier, int maxTier)
{
    return pickGear(cls, ItemType::Weapon, floor, minTier, maxTier);
}

Item DungeonPopulator::pickArmor(PlayerClass cls, int floor, int minTier, int maxTier)
{
    return pickGear(cls, ItemType::Armor, floor, minTier, maxTier);
}

int DungeonPopulator::tierForItemName(const std::string& name)
{
    for (const auto& g : gearTable())
        if (g.item.name == name) return g.item.tier;
    // Nombres antiguos sin tilde
    if (name == "Varita de Sauco")    return 1;
    if (name == "Baculo de Cristal" || name == "Baculo de Rayos" ||
        name == "Manto Mistico"     || name == "Arco Elfico") return 2;
    if (name == "Grimorio Demoniaco") return 3;
    return 1;
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

Item DungeonPopulator::pickShovel(int floor)
{
    int price = 200 + floor * 50;
    return {"Pala", "Excava hacia el siguiente piso.", ItemType::Consumable, price, 1, 0, 1};
}

// ─── Position helper ──────────────────────────────────────────────────────────

Position DungeonPopulator::pickPos(const BSPDungeon::Room& r, std::vector<Position>& taken,
                                   std::mt19937& rng)
{
    int iw = std::max(1, r.w - 2);
    int ih = std::max(1, r.h - 2);
    for (int attempt = 0; attempt < 30; attempt++) {
        Position p = {r.x + 1 + rngInt(rng, 0, iw - 1),
                      r.y + 1 + rngInt(rng, 0, ih - 1)};
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

static void collectSecretCandidates(Map& map,
                                    const std::vector<BSPDungeon::Room>& rooms,
                                    std::vector<SecretCand>& out)
{
    static const int DIRS[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    int w = map.width(), h = map.height();

    // Build room mask to skip interior tiles
    std::vector<bool> isRoom(w * h, false);
    for (auto& r : rooms)
        for (int y = r.y; y < r.y + r.h; y++)
            for (int x = r.x; x < r.x + r.w; x++)
                isRoom[y * w + x] = true;

    // Scan room perimeters + corridor tiles (floor not in rooms)
    for (int y = 2; y < h - 2; y++) {
        for (int x = 2; x < w - 2; x++) {
            if (map.at(x, y).type != TileType::Floor) continue;
            for (auto& d : DIRS) {
                int wx = x + d[0], wy = y + d[1];
                if (wx < 1 || wx >= w - 1 || wy < 1 || wy >= h - 1) continue;
                if (map.at(wx, wy).type == TileType::Wall)
                    out.push_back({wx, wy, d[0], d[1]});
            }
        }
    }
}

void DungeonPopulator::tryPlaceSecretRoom(Map& map, std::vector<WorldChest>& chests,
                                          std::vector<Position>& taken,
                                          PlayerClass cls, int floor,
                                          std::vector<SecretCand>& cands,
                                          std::mt19937& rng)
{
    static const int INNER = 3;

    // Fisher-Yates shuffle using RNG
    for (int i = static_cast<int>(cands.size()) - 1; i > 0; i--)
        std::swap(cands[i], cands[rngInt(rng, 0, i)]);

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
        Item item = (std::rand() % 2 == 0) ? pickWeapon(cls, floor, 2) : pickArmor(cls, floor, 2);
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

DungeonPopulator::Result DungeonPopulator::populate(
    Map& map, const std::vector<BSPDungeon::Room>& rooms, int depth, PlayerClass cls,
    int incursions, std::mt19937& rng)
{
    Result result;
    int n = static_cast<int>(rooms.size());

    std::vector<Position> taken;
    taken.push_back({rooms[0].centerX(), rooms[0].centerY()});

    if (n >= 3) {
        result.lockedDoorPos    = pickPos(rooms[n - 2], taken, rng);
        result.lockedDoorExists = true;
        result.stairsPos        = result.lockedDoorPos;
    } else {
        result.stairsPos = pickPos(rooms[n - 1], taken, rng);
    }

    // Escalera de subida: se coloca en la sala donde entras al piso.
    // Solo se puede usar cuando todos los enemigos del piso están muertos.
    result.stairsUpPos   = pickPos(rooms[0], taken, rng);
    result.stairsUpExists = true;

    // Enemies
    int spawnChance = std::min(85, 50 + (depth - 1) * 8);
    for (int i = 1; i < n; i++) {
        if (rngInt(rng, 0, 99) < spawnChance)
            result.enemies.push_back({pickPos(rooms[i], taken, rng), pickPos(rooms[i], taken, rng),
                                      pickEnemyType(depth, rng), true});
        if (rngInt(rng, 0, 99) < 15 + depth * 3)
            result.enemies.push_back({pickPos(rooms[i], taken, rng), pickPos(rooms[i], taken, rng),
                                      pickEnemyType(depth, rng), true});
        if (depth >= 2 && rngInt(rng, 0, 99) < depth * 3)
            result.enemies.push_back({pickPos(rooms[i], taken, rng), pickPos(rooms[i], taken, rng),
                                      pickEnemyType(depth, rng), true});
    }

    // Chests
    int chestCount = 2 + rngInt(rng, 0, 2);
    std::vector<int> eligible;
    for (int i = 1; i < n - 1; i++) eligible.push_back(i);
    for (int i = static_cast<int>(eligible.size()) - 1; i > 0; i--)
        std::swap(eligible[i], eligible[rngInt(rng, 0, i)]);
    chestCount = std::min(chestCount, static_cast<int>(eligible.size()));

    float coinFactor = 1.0f + (incursions - 1) * 0.15f;
    for (int ci = 0; ci < chestCount; ci++) {
        int roll = rngInt(rng, 0, 99);
        ChestLoot loot;
        int coins = 0;
        Item item{};
        int baseCoins = 10 + rngInt(rng, 0, 39);
        if (roll < 55) {
            loot  = ChestLoot::Coins;
            coins = static_cast<int>(baseCoins * (1 + (depth - 1) / 2) * coinFactor);
        } else {
            loot     = ChestLoot::Item;
            int r    = rngInt(rng, 0, 3);
            if      (r == 0) item = pickPotion(depth);
            else if (r == 1) item = pickWeapon(cls, depth);
            else             item = pickArmor(cls, depth);
        }
        result.chests.push_back({pickPos(rooms[eligible[ci]], taken, rng), false, loot, coins, item});
    }

    // Mimics
    for (auto& ch : result.chests)
        if (!ch.opened && rngInt(rng, 0, 99) < 12)
            ch.isMimic = true;

    // Secret rooms — collect candidates once, try up to 2 placements
    std::vector<SecretCand> secCands;
    collectSecretCandidates(map, rooms, secCands);

    int numSecret = 1 + rngInt(rng, 0, 1);
    for (int i = 0; i < numSecret; i++)
        tryPlaceSecretRoom(map, result.chests, taken, cls, depth, secCands, rng);

    // Boss every 5 floors
    if (depth % 5 == 0 && depth > 0 && n >= 3)
        result.enemies.push_back({pickPos(rooms[n - 2], taken, rng), pickPos(rooms[n - 2], taken, rng),
                                  pickEnemyType(depth, rng), true, true});

    // Torches — decoración en salas (no en spawn)
    for (int i = 1; i < n - 1; i++) {
        if (rngInt(rng, 0, 99) < 70) {
            Position tpos;
            if (rooms[i].w >= 6 && rooms[i].h >= 6) {
                int corner = rngInt(rng, 0, 3);
                switch (corner) {
                    case 0: tpos = {rooms[i].x + 1,         rooms[i].y + 1};         break;
                    case 1: tpos = {rooms[i].x + rooms[i].w - 2, rooms[i].y + 1};    break;
                    case 2: tpos = {rooms[i].x + 1,         rooms[i].y + rooms[i].h - 2}; break;
                    case 3: tpos = {rooms[i].x + rooms[i].w - 2, rooms[i].y + rooms[i].h - 2}; break;
                }
            } else {
                tpos = {rooms[i].centerX(), rooms[i].centerY()};
            }
            bool ok = true;
            for (const auto& t : taken)
                if (t.x == tpos.x && t.y == tpos.y) { ok = false; break; }
            if (ok) {
                taken.push_back(tpos);
                result.torches.push_back({tpos});
            }
        }
    }

    return result;
}
