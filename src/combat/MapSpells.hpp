#pragma once

// MapSpells — habilidades del árbol que también se usan en el mapa (fuera de
// combate), con la tecla Q. La mayoría son proyectiles; las selfCast estallan
// alrededor del jugador sin apuntar. Se desbloquean con la misma habilidad del
// árbol: tener el efecto en Player::hasSkill() habilita su versión de mapa.

#include "Art.hpp"
#include "entities/Player.hpp"
#include <raylib.h>

// DEPURACIÓN: true desbloquea todas las habilidades del mapa (hechizos y trampas)
// para cualquier clase y nivel. Dejar en false para jugar normal.
inline constexpr bool kDebugUnlockMapAbilities = true;

// true si el jugador puede usar en el mapa la habilidad con ese efecto.
inline bool canUseMapAbility(const Player& p, ArtEffect effect) {
    return kDebugUnlockMapAbilities || p.hasSkill(effect);
}

enum class MapFxStyle { Fire, Ice, Lightning, Drain, Impact };

struct MapSpellDef {
    ArtEffect   effect;
    const char* name;
    int         range;       // distancia Chebyshev máxima al objetivo
    int         radius;      // 0 = un solo objetivo; 1 = también las 8 casillas vecinas
    float       speed;       // casillas por segundo del proyectil
    int         glyph;       // glifo del proyectil
    int         colorPair;   // color de la vista previa (Renderer::colorFromPair)
    Color       color;       // color del proyectil
    MapFxStyle  style;       // estilo del destello al impactar
    int         pctNormal;   // daño como % de la vida máxima del enemigo
    int         pctBoss;     // idem contra jefes
    int         atkMult2;    // + ATK * atkMult2 / 2 de daño plano
    int         freezeTicks; // ticks de IA que el enemigo queda congelado
    int         drainPct;    // % del daño infligido que cura al jugador
    bool        selfCast = false; // sin proyectil: afecta el radio alrededor del jugador
};

inline constexpr MapSpellDef kMapSpells[] = {
    { ArtEffect::BolaDeFuego, "Bola de fuego",  8, 1, 16.0f, 0x25CF, 9, {255, 140,  40, 255},
      MapFxStyle::Fire,      30, 10, 2, 0,  0 },
    { ArtEffect::Congelar,    "Congelar",       8, 0, 20.0f, 0x25C6, 3, { 90, 220, 255, 255},
      MapFxStyle::Ice,       10,  5, 1, 6,  0 },
    { ArtEffect::Drenaje,     "Drenaje",        6, 0, 12.0f, 0x25CB, 8, {220,  80, 255, 255},
      MapFxStyle::Drain,     30, 10, 2, 0, 50 },
    { ArtEffect::Rayo,        "Rayo",          10, 0, 60.0f, '+',    2, {255, 240, 120, 255},
      MapFxStyle::Lightning, 50, 20, 4, 0,  0 },
    { ArtEffect::GritoDeGuerra, "Grito de guerra", 0, 2, 0.0f, '!',  2, {255, 220,  90, 255},
      MapFxStyle::Lightning,  5,  2, 1, 5,  0, true },
    { ArtEffect::LluviaEstelar, "Lluvia estelar",  8, 1, 14.0f, '*', 5, {255, 255, 200, 255},
      MapFxStyle::Lightning, 30, 10, 2, 0,  0 },
    { ArtEffect::DistorsionTemporal, "Distorsión temporal", 8, 1, 10.0f, 0x25CE, 8, {170, 120, 255, 255},
      MapFxStyle::Drain,     10,  5, 1, 6,  0 },
};
inline constexpr int kMapSpellCount = sizeof(kMapSpells) / sizeof(kMapSpells[0]);

inline bool hasAnyMapSpell(const Player& p) {
    for (const auto& s : kMapSpells)
        if (canUseMapAbility(p, s.effect)) return true;
    return false;
}
