#pragma once

// Habilidades especiales (Arts): 3 por clase, definidas en Player::getAvailableArts().
// La lógica de resolución está en CombatSystem::resolveArt().
// Para añadir una habilidad nueva: agregar valor aquí, caso en resolveArt() y entrada en getAvailableArts().

#include <string>

enum class ArtEffect {
    GolpeDemoledor,  // Guerrero: ATK*1.4 ignorando mitad de la DEF enemiga
    GritoDeGuerra,   // Guerrero: +ATK durante 2 turnos
    EscudoTotal,     // Guerrero: -70% en el próximo golpe recibido (consumible)
    BolaDeFuego,     // Mago: daño mágico igual a ATK+10 a TODOS los enemigos
    Congelar,        // Mago: enemigo objetivo pierde 1 PA en su próximo turno
    Drenaje,         // Mago: daño + recupera HP igual al daño infligido
    DisparoDoble,    // Ranger: 2 ataques normales seguidos al objetivo
    Trampa,          // Ranger: daño retardado (se aplica al inicio del siguiente turno enemigo)
    Veneno,          // Ranger: DoT 3 turnos (magnitud = ATK/3)
    LluviaEstelar,   // Halley: ATK*1.5 raw dmg a TODOS los enemigos
    Creacion,        // Halley: recupera 50% HP máximo
    PresenciaDivina, // Halley: +ATK/2+5 por 3 turnos
    DistorsionTemporal, // Nato: enemigo pierde TODOS sus PA este turno
    VinculoEterno,   // Nato: daño ATK + cura 150% del daño infligido
    JuicioFinal,     // Nato: ATK*2.0 raw dmg ignora DEF
};

struct Art {
    std::string name;
    int         apCost;
    int         manaCost;
    ArtEffect   effect;
    std::string description;
};
