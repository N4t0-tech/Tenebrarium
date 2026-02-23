#pragma once

#include <string>

enum class ArtEffect {
    GolpeDemoledor,  // Warrior: direct ATK ignoring DEF
    GritoDeGuerra,   // Warrior: +ATK for 2 turns
    EscudoTotal,     // Warrior: -70% next hit received
    BolaDeFuego,     // Mage: magic damage to ALL enemies
    Congelar,        // Mage: enemy loses 1 PA this turn
    Drenaje,         // Mage: damage + recover HP equal to damage
    DisparoDoble,    // Ranger: 2 quick attacks
    Trampa,          // Ranger: delayed damage next turn
    Veneno,          // Ranger: DoT 3 turns
};

struct Art {
    std::string name;
    int         apCost;
    int         manaCost;
    ArtEffect   effect;
    std::string description;
};
