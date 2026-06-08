#pragma once

#include "../entities/Enemy.hpp"
#include <string>
#include <array>

struct BestiaryEntry {
    EnemyType type;
    std::string name;
    int kills;
    int encountered;
    int firstSeenFloor;
    bool discovered;
};

struct BestiaryInfo {
    std::string name;
    int baseHp;
    int baseAtk;
    int baseDef;
    int basePa;
    int firstFloor;
    int glyph;
    int colorPair;
    std::string special;
};

static constexpr int kBestiaryEntryCount = 10;

static const std::array<BestiaryInfo, kBestiaryEntryCount> kBestiaryData{{
    {"Goblin",   125, 17,  4, 1, 1, 'g',      10, ""},
    {"Esqueleto",120, 16,  4, 1, 1, 's',       7, ""},
    {"Orco",     140, 20, 10, 2, 2, 'o',       9, "2 PA"},
    {"Araña",     80, 12,  2, 1, 2, 'a',       8, "Veneno al golpear"},
    {"Vampiro",  110, 18,  6, 1, 4, 'V',       6, "Robavida 30%"},
    {"Zombie",   160, 10, 15, 1, 2, 'z',      10, "Tanque (DEF alta)"},
    {"Demonio",  150, 25,  8, 1, 6, 'd',       6, ""},
    {"Sombra",    70, 18,  2, 2, 4, 'S',      11, "Doble ataque (2 PA)"},
    {"Mímico",   110, 18, 10, 1, 1, 'M',       9, "Solo en cofres"},
    {"Slime",     60,  8,  1, 1, 1, 0x25CF,   10, "Se divide al morir"},
}};
