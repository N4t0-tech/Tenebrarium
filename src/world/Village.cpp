#include "Village.hpp"
#include "ui/XpLoader.hpp"

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

} // namespace

VillageLayout buildVillage()
{
    VillageLayout v;

    std::string path = "assets/maps/aldea.xp";
    const XpFile& xp = loadXpCached(path);
    const XpLayer& layer = xp.layers[0];

    int w = layer.width, h = layer.height;

    auto set = [&](int x, int y, TileType t, char32_t g, const XpCell& c) {
        bool empty = (g == U' '); // celdas vacías del arte (fondo magenta REXPaint)
        uint8_t z = 0;
        v.map.at(x, y) = { t, g, true, true,
                           empty ? z : c.fg_r, empty ? z : c.fg_g, empty ? z : c.fg_b,
                           empty ? z : c.bg_r, empty ? z : c.bg_g, empty ? z : c.bg_b };
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const XpCell& c = layer.cells[y * w + x];
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

    v.map.setPlayerPos(v.spawn.x, v.spawn.y);
    return v;
}