#include "Village.hpp"
#include "ui/XpLoader.hpp"
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <queue>

// La aldea se construye desde el arte REXPaint assets/maps/aldea.xp.
// Celdas del arte → tiles:
//   ░ y ◙            → suelo caminable (◙ marca el spawn)
//   letras T F E v i  → suelo caminable + posición funcional
//                       (tienda, forja, entrenamiento, escalera, farola)
//   cualquier otro    → bloqueante (muros, setos, agua, postes…) conservando
//                       glifo y colores del arte
namespace {

bool isTileFloor(char32_t g)
{
    return g == 0x2591 || g == 0x25D9 || g == 0x25D8 ||
           g == 0x2193 || g == 0x2588 ||
           g == 0x2584 || g == 0x258C || g == 0x2590 ||
           g == 'T' || g == 'F' || g == 'E' || g == 'v' || g == 'i';
}

// Plaza amurallada usada cuando el arte no se puede cargar: el juego sigue
// siendo jugable y los puntos funcionales caen en sus posiciones por defecto.
void buildFallback(VillageLayout& v)
{
    v.map = Map(60, 60);
    for (int y = 0; y < v.map.height(); y++) {
        for (int x = 0; x < v.map.width(); x++) {
            bool border = x == 0 || y == 0 || x == v.map.width() - 1 || y == v.map.height() - 1;
            v.map.at(x, y) = border
                ? Tile{ TileType::Wall,  U'#',    true, true, 140, 140, 140, 0, 0, 0 }
                : Tile{ TileType::Floor, 0x2591, true, true,  50,  50,  50, 0, 0, 0 };
        }
    }
}


// ─── Decoración procedural ───────────────────────────────────────────────────
// Sobre el arte se pinta textura de hierba, caminos de tierra desde la plaza
// hasta cada puerta, suelos interiores según el edificio y carteles. Todo es
// determinista: el mismo arte produce siempre la misma aldea.

uint32_t hash32(int x, int y, uint32_t seed)
{
    uint32_t v = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u
               + seed * 2246822519u;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}

float rnd(int x, int y, uint32_t seed) { return hash32(x, y, seed) / 4294967296.0f; }

// Ruido de valor suavizado: manchas de unas `scale` casillas
float smoothNoise(int x, int y, float scale, uint32_t seed)
{
    float fx = x / scale, fy = y / scale;
    int x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
    float tx = fx - x0, ty = fy - y0;
    tx = tx * tx * (3 - 2 * tx);
    ty = ty * ty * (3 - 2 * ty);
    float a = rnd(x0, y0, seed),     b = rnd(x0 + 1, y0, seed);
    float c = rnd(x0, y0 + 1, seed), d = rnd(x0 + 1, y0 + 1, seed);
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty;
}

struct Rgb { int r, g, b; };

Rgb mix(Rgb a, Rgb b, float t)
{
    return { static_cast<int>(a.r + (b.r - a.r) * t), static_cast<int>(a.g + (b.g - a.g) * t),
             static_cast<int>(a.b + (b.b - a.b) * t) };
}

Rgb lighten(Rgb c, int d)
{
    auto cl = [](int v) { return std::clamp(v, 0, 255); };
    return { cl(c.r + d), cl(c.g + d), cl(c.b + d) };
}

void paint(Tile& t, char32_t glyph, Rgb fg, Rgb bg)
{
    t.glyph = glyph;
    t.fg_r = static_cast<uint8_t>(fg.r); t.fg_g = static_cast<uint8_t>(fg.g); t.fg_b = static_cast<uint8_t>(fg.b);
    t.bg_r = static_cast<uint8_t>(bg.r); t.bg_g = static_cast<uint8_t>(bg.g); t.bg_b = static_cast<uint8_t>(bg.b);
}

void paintGrass(Tile& t, int x, int y)
{
    static constexpr Rgb kGrass[3] = { {20, 52, 27}, {25, 62, 32}, {31, 73, 38} };
    float v = smoothNoise(x, y, 6.0f, 1) * 0.75f + rnd(x, y, 2) * 0.25f;
    Rgb bg = v < 0.5f ? mix(kGrass[0], kGrass[1], std::min(1.0f, v * 2))
                      : mix(kGrass[1], kGrass[2], (v - 0.5f) * 2);
    float r = rnd(x, y, 5);
    if (r < 0.62f) {
        paint(t, U' ', lighten(bg, 20), bg);
    } else if (r < 0.94f) {
        static constexpr char32_t kBlades[] = { U',', U'\'', U'"', U'`', U'.' };
        paint(t, kBlades[hash32(x, y, 6) % 5], lighten(bg, 30), bg);
    } else if (r < 0.97f) {
        paint(t, U'"', lighten(bg, 48), bg);
    } else {
        static constexpr Rgb kFlowers[3] = { {200, 70, 70}, {220, 200, 90}, {200, 200, 220} };
        paint(t, 0x2219, kFlowers[hash32(x, y, 8) % 3], bg);
    }
}

void decorate(VillageLayout& v)
{
    Map& m = v.map;
    const int w = m.width(), h = m.height();
    auto inside = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h; };
    auto idx    = [&](int x, int y) { return y * w + x; };
    auto walk   = [&](int x, int y) { return inside(x, y) && m.isWalkable(x, y); };
    auto wall   = [&](int x, int y) { return inside(x, y) && m.at(x, y).glyph == U'#'; };
    static constexpr int kDx[4] = { 1, -1, 0, 0 };
    static constexpr int kDy[4] = { 0, 0, 1, -1 };
    if (!walk(v.spawn.x, v.spawn.y)) return;

    // Puertas: casillas transitables entre dos muros '#' opuestos
    std::vector<char> door(w * h, 0);
    std::vector<Position> doors;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (walk(x, y) && ((wall(x - 1, y) && wall(x + 1, y)) || (wall(x, y - 1) && wall(x, y + 1)))) {
                door[idx(x, y)] = 1;
                doors.push_back({x, y});
            }

    // Exterior: lo alcanzable desde el spawn sin cruzar puertas
    std::vector<char> outside(w * h, 0);
    std::vector<Position> stack{ v.spawn };
    outside[idx(v.spawn.x, v.spawn.y)] = 1;
    while (!stack.empty()) {
        Position p = stack.back(); stack.pop_back();
        for (int d = 0; d < 4; d++) {
            int nx = p.x + kDx[d], ny = p.y + kDy[d];
            if (walk(nx, ny) && !outside[idx(nx, ny)] && !door[idx(nx, ny)]) {
                outside[idx(nx, ny)] = 1;
                stack.push_back({nx, ny});
            }
        }
    }

    // Interiores: cada región cerrada restante es un edificio
    enum class Kind { Plain, Shop, Forge, Training, Stairs };
    struct Region { Kind kind = Kind::Plain; std::vector<Position> cells; Position door{-1, -1}; };
    std::vector<Region> regions;
    std::vector<int> regionOf(w * h, -1);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int i = idx(x, y);
            if (!walk(x, y) || outside[i] || door[i] || regionOf[i] >= 0) continue;
            Region r;
            int id = static_cast<int>(regions.size());
            stack = { {x, y} };
            regionOf[i] = id;
            while (!stack.empty()) {
                Position p = stack.back(); stack.pop_back();
                r.cells.push_back(p);
                for (int d = 0; d < 4; d++) {
                    int nx = p.x + kDx[d], ny = p.y + kDy[d];
                    if (!walk(nx, ny)) continue;
                    int ni = idx(nx, ny);
                    if (door[ni]) { r.door = {nx, ny}; continue; }
                    if (outside[ni] || regionOf[ni] >= 0) continue;
                    regionOf[ni] = id;
                    stack.push_back({nx, ny});
                }
            }
            auto has = [&](Position q) { return inside(q.x, q.y) && regionOf[idx(q.x, q.y)] == id; };
            if      (has(v.shop))       r.kind = Kind::Shop;
            else if (has(v.forge))      r.kind = Kind::Forge;
            else if (has(v.training))   r.kind = Kind::Training;
            else if (has(v.stairsDown)) r.kind = Kind::Stairs;
            regions.push_back(std::move(r));
        }
    }

    // Caminos: Dijkstra desde el spawn con coste ruidoso para que serpenteen un
    // poco; las casillas que ya son camino salen baratas y los ramales se unen.
    std::vector<char> path(w * h, 0);
    auto route = [&](Position goal) {
        std::vector<float> dist(w * h, 1e9f);
        std::vector<int> prev(w * h, -1);
        using Node = std::pair<float, int>;
        std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
        int start = idx(v.spawn.x, v.spawn.y), target = idx(goal.x, goal.y);
        dist[start] = 0;
        pq.push({0.0f, start});
        while (!pq.empty()) {
            auto [d, i] = pq.top(); pq.pop();
            if (i == target) break;
            if (d > dist[i]) continue;
            int px = i % w, py = i / w;
            for (int k = 0; k < 4; k++) {
                int nx = px + kDx[k], ny = py + kDy[k];
                if (!walk(nx, ny)) continue;
                int ni = idx(nx, ny);
                if (!outside[ni] && ni != target) continue;
                float c = 1.0f + 0.8f * smoothNoise(nx, ny, 4.0f, 7);
                if (path[ni]) c *= 0.35f;
                for (int e = 0; e < 4; e++)
                    if (!walk(nx + kDx[e], ny + kDy[e])) c += 0.6f;  // no rozar muros ni árboles
                if (d + c < dist[ni]) {
                    dist[ni] = d + c;
                    prev[ni] = i;
                    pq.push({d + c, ni});
                }
            }
        }
        if (prev[target] < 0) return;
        for (int i = prev[target]; i >= 0 && i != start; i = prev[i])
            path[i] = 1;
    };
    std::sort(doors.begin(), doors.end(), [&](Position a, Position b) {
        auto md = [&](Position p) { return std::abs(p.x - v.spawn.x) + std::abs(p.y - v.spawn.y); };
        return md(a) < md(b);
    });
    for (Position d : doors) route(d);

    auto isPlaza = [&](int x, int y) {
        float dx = static_cast<float>(x - v.spawn.x), dy = (y - v.spawn.y) * 1.8f;
        return outside[idx(x, y)] && dx * dx + dy * dy <= 10.0f;
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Tile& t = m.at(x, y);
            int i = idx(x, y);
            if (door[i]) {
                paint(t, U' ', {0, 0, 0}, {84, 62, 38});
            } else if (outside[i] && t.glyph != 0x2193) {
                if (isPlaza(x, y)) {
                    Rgb bg = (x + y) % 2 ? Rgb{72, 68, 60} : Rgb{66, 62, 55};
                    paint(t, rnd(x, y, 9) < 0.5f ? 0x2591 : U' ', lighten(bg, 12), bg);
                } else if (path[i]) {
                    Rgb bg = mix({82, 64, 40}, {70, 54, 34}, rnd(x, y, 10));
                    paint(t, rnd(x, y, 11) < 0.7f ? U' ' : U'.', lighten(bg, 22), bg);
                } else {
                    bool nearPath = false;
                    for (int d = 0; d < 4; d++) {
                        int nx = x + kDx[d], ny = y + kDy[d];
                        if (inside(nx, ny) && path[idx(nx, ny)]) nearPath = true;
                    }
                    paintGrass(t, x, y);
                    // Bordes del camino: hierba pisada que se funde con la tierra
                    if (nearPath && rnd(x, y, 3) < 0.45f) {
                        Rgb bg = mix({t.bg_r, t.bg_g, t.bg_b}, {76, 60, 38}, 0.55f);
                        paint(t, t.glyph == 0x2219 ? U' ' : t.glyph, lighten(bg, 20), bg);
                    }
                }
            }
        }
    }

    // Árboles: su fondo pasa a ser la hierba del lugar, algo más oscura
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Tile& t = m.at(x, y);
            if ((t.glyph == 0x25B2 || t.glyph == U'I') && t.bg_r == 0 && t.bg_g == 64 && t.bg_b == 48) {
                Tile grass = t;
                paintGrass(grass, x, y);
                Rgb bg = mix({grass.bg_r, grass.bg_g, grass.bg_b}, {0, 0, 0}, 0.25f);
                paint(t, t.glyph, {t.fg_r, t.fg_g, t.fg_b}, bg);
            }
        }
    }

    static const char* kSignText[] = { "", "Tienda", "Forja", "Entrenamiento", "Descenso" };
    for (const Region& r : regions) {
        for (Position p : r.cells) {
            Tile& t = m.at(p.x, p.y);
            bool stairs = (t.glyph == 0x2193);
            switch (r.kind) {
            case Kind::Shop:
            case Kind::Training: {
                Rgb bg = p.y % 2 ? Rgb{62, 44, 28} : Rgb{56, 40, 25};
                paint(t, rnd(p.x, p.y, 12) < 0.35f ? 0x2500 : U' ', lighten(bg, -14), bg);
                break;
            }
            case Kind::Forge: {
                Rgb bg = mix({48, 48, 52}, {40, 40, 44}, rnd(p.x, p.y, 13));
                paint(t, rnd(p.x, p.y, 14) < 0.5f ? 0x2591 : U' ', lighten(bg, 14), bg);
                break;
            }
            case Kind::Stairs: {
                Rgb bg = mix({34, 26, 44}, {28, 22, 36}, rnd(p.x, p.y, 15));
                if (stairs) paint(t, 0x2193, {178, 0, 45}, bg);
                else        paint(t, rnd(p.x, p.y, 16) < 0.45f ? 0x2591 : U' ', {52, 40, 66}, bg);
                break;
            }
            case Kind::Plain:
                break;
            }
        }
        if (r.kind == Kind::Plain || r.cells.empty()) continue;
        int minX = w, maxX = 0, minY = h;
        for (Position p : r.cells) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
        }
        // Sobre la fachada; si ahí está la entrada, una fila más arriba para no taparla
        Position signPos{ (minX + maxX) / 2, minY - 1 };
        if (signPos.x == r.door.x && signPos.y == r.door.y && signPos.y > 0) signPos.y--;
        v.signs.push_back({ signPos, r.door,
                            kSignText[static_cast<int>(r.kind)] });
    }
}

} // namespace

int forgeCost(int statBonus, int incursions)
{
    return 50 + 25 * statBonus + 20 * (std::max(1, incursions) - 1);
}

int trainingCost(int option, int incursions)
{
    const int inc = std::max(1, incursions);
    switch (option) {
    case 0:  return 60 + 20 * inc;
    case 1:  return 80 + 30 * inc;
    case 2:  return 70 + 25 * inc;
    default: return 50 + 20 * inc;
    }
}

VillageLayout buildVillage()
{
    VillageLayout v;

    const std::string path = "assets/maps/aldea.xp";
    const XpLayer* layer = nullptr;
    try {
        const XpFile& xp = loadXpCached(path);
        if (!xp.layers.empty()) {
            const XpLayer& l = xp.layers[0];
            if (l.width > 0 && l.height > 0
                && l.cells.size() >= static_cast<size_t>(l.width) * l.height)
                layer = &l;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "Aldea: %s", e.what());
    }

    if (!layer) {
        TraceLog(LOG_WARNING, "Aldea: arte no disponible, usando mapa de respaldo");
        buildFallback(v);
    }

    const int w = layer ? layer->width : 0;
    const int h = layer ? layer->height : 0;
    if (layer)
        v.map = Map(w, h);

    auto set = [&](int x, int y, TileType t, char32_t g, const XpCell& c) {
        bool empty = (g == U' '); // celdas vacías del arte (fondo magenta REXPaint)
        uint8_t z = 0;
        v.map.at(x, y) = { t, g, true, true,
                           empty ? z : c.fg_r, empty ? z : c.fg_g, empty ? z : c.fg_b,
                           empty ? z : c.bg_r, empty ? z : c.bg_g, empty ? z : c.bg_b };
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const XpCell& c = layer->cells[y * w + x];
            char32_t g = c.glyph;

            switch (g) {
            case 'T': v.shop = {x, y};      set(x, y, TileType::Floor, 0x2591, c); break;
            case 'F': v.forge = {x, y};     set(x, y, TileType::Floor, 0x2591, c); break;
            case 'E': v.training = {x, y};  set(x, y, TileType::Floor, 0x2591, c); break;
            case 'v':
                v.stairsDown = {x, y};
                set(x, y, TileType::Floor, 0x2591, c);
                break;
            case 0x2193:
                v.stairsDown = {x, y};
                set(x, y, TileType::Floor, 0x2193, c);
                break;
            case 'i':
                v.lanterns.push_back({x, y});
                set(x, y, TileType::Floor, 0x2591, c);
                break;
            default:
                set(x, y, isTileFloor(g) ? TileType::Floor : TileType::Wall, g, c);
                if (g == 0x25D9 || g == 0x25D8) v.spawn = {x, y};
                break;
            }
        }
    }

    // Valores por defecto si el arte aún no marca algún punto funcional
    if (v.spawn.x == 0 && v.spawn.y == 0)      v.spawn      = {29, 23};
    if (v.shop.x == 0 && v.shop.y == 0)        v.shop       = {14, 24};
    if (v.forge.x == 0 && v.forge.y == 0)      v.forge      = {42, 22};
    if (v.training.x == 0 && v.training.y == 0) v.training  = {20, 13};
    if (v.stairsDown.x == 0 && v.stairsDown.y == 0) v.stairsDown = {43, 23};
    if (v.lanterns.empty()) {
        const Position lanternDefaults[] = {{26, 23}, {32, 23}};
        for (int i = 0; i < 2; i++)
            v.lanterns.push_back(lanternDefaults[i]);
    }

    // Un arte más pequeño que los valores por defecto no debe dejar puntos fuera del mapa
    auto clampPos = [&](Position& p) {
        p.x = std::clamp(p.x, 0, v.map.width() - 1);
        p.y = std::clamp(p.y, 0, v.map.height() - 1);
    };
    clampPos(v.spawn);
    clampPos(v.shop);
    clampPos(v.forge);
    clampPos(v.training);
    clampPos(v.stairsDown);
    v.lanterns.erase(std::remove_if(v.lanterns.begin(), v.lanterns.end(), [&](Position p) {
        return p.x < 0 || p.y < 0 || p.x >= v.map.width() || p.y >= v.map.height();
    }), v.lanterns.end());

    decorate(v);
    v.map.setPlayerPos(v.spawn.x, v.spawn.y);
    return v;
}