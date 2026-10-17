#pragma once

// Pueblo hub: mapa fijo y pequeño donde el jugador invierte lo que saca de la
// mazmorra. Estructuras: tienda, forja, entrenamiento y la escalera de bajada.
// Todas las casillas se marcan exploradas: en el hub no hay niebla de guerra.

#include "Map.hpp"
#include <string>
#include <vector>

enum class VillageMenu {
    None,     // caminando por el pueblo
    Forge,    // submenú de la forja (mejorar equipo)
    Training, // submenú de entrenamiento (mejorar stats)
};

// Cartel sobre la fachada de un edificio. Se resalta cuando el jugador se
// acerca a su puerta.
struct VillageSign {
    Position    pos;    // casilla central del cartel
    Position    door;   // entrada del edificio
    std::string text;
};

struct VillageLayout {
    Map      map{60, 60};        // se redimensiona al tamaño del arte
    Position spawn{0, 0};        // posición inicial del jugador
    Position shop{0, 0};         // casilla que abre la tienda (estado Shop)
    Position forge{0, 0};        // casilla que abre el submenú de forja
    Position training{0, 0};     // casilla que abre el submenú de entrenamiento
    Position stairsDown{0, 0};   // casilla que inicia una nueva incursión
    std::vector<Position> lanterns;    // farolas decorativas (luz cálida)
    std::vector<VillageSign> signs;    // carteles de los edificios
};

// Si assets/maps/aldea.xp falta o está corrupto, construye una plaza de respaldo.
VillageLayout buildVillage();

// Costes de la forja y el entrenamiento. Única fuente de verdad para lo que se
// muestra y lo que se cobra. Antes de la primera incursión cuenta como la 1.
inline constexpr int kTrainingOptions = 4;
int forgeCost(int statBonus, int incursions);
int trainingCost(int option, int incursions);
int artRankCost(int nextRank, int incursions);  // subir una habilidad al rango nextRank
