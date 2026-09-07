#pragma once

// Pueblo hub: mapa fijo y pequeño donde el jugador invierte lo que saca de la
// mazmorra. Estructuras: tienda, forja, entrenamiento y la escalera de bajada.
// Todas las casillas se marcan exploradas: en el hub no hay niebla de guerra.

#include "Map.hpp"
#include <vector>

enum class VillageMenu {
    None,     // caminando por el pueblo
    Forge,    // submenú de la forja (mejorar equipo)
    Training, // submenú de entrenamiento (mejorar stats)
};

struct VillageLayout {
    Map      map{60, 60};
    Position spawn{0, 0};        // posición inicial del jugador
    Position shop{0, 0};         // casilla que abre la tienda (estado Shop)
    Position forge{0, 0};        // casilla que abre el submenú de forja
    Position training{0, 0};     // casilla que abre el submenú de entrenamiento
    Position stairsDown{0, 0};   // casilla que inicia una nueva incursión
    std::vector<Position> lanterns;    // farolas decorativas (luz cálida)
};

VillageLayout buildVillage();