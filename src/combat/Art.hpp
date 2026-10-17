#pragma once

// Habilidades especiales (Arts). Cada clase tiene 3 base y 6 en su árbol (SkillTree.cpp);
// el jugador lleva 3 equipadas (Player::getAvailableArts()).
// La lógica de resolución está en CombatSystem::resolveArt().
// Para añadir una habilidad nueva: agregar valor AL FINAL (el guardado usa el número),
// caso en resolveArt() y entrada en SkillTree.cpp.

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
    // Árbol de habilidades
    BerserkWarrior,      // Guerrero: +50% ATK y -30% DEF 3 turnos
    Fortaleza,           // Guerrero: regenera 10% HP por turno 3 turnos
    Rayo,                // Mago: ATK*1.8 ignora DEF
    TormentaArcana,      // Mago: ATK/2 a todos + congela
    EscudoArcano,        // Mago: absorbe el próximo golpe
    LluviaFlechas,       // Arquera: 3 flechas a enemigos al azar
    GolpeCritico,        // Arquera (Tiro Certero): ATK*2.5
    Sombra,              // Arquera (Paso Sombrío): esquiva el próximo golpe y contraataca
    AbsorbVida,          // Halley: roba 30% de la vida actual del enemigo
    CampoEquilibrio,     // Halley: iguala la vida con el enemigo
    ColapsoDimensional,  // Halley: daño y baja ATK/DEF del enemigo
    Terremoto,           // Guerrero: ATK*1.2 a todos
    Ejecutar,            // Guerrero: ATK*1.5, x3 si el enemigo tiene <30% de vida
    Rompecorazas,        // Guerrero: daño y -50% DEF del enemigo 3 turnos
    UltimoBastion,       // Guerrero: no baja de 1 HP durante 2 turnos
    CadenaRayos,         // Mago: rayo que salta a hasta 3 enemigos
    Meteoro,             // Mago: ATK*2.2 a todos
    Maldicion,           // Mago: enemigo recibe +30% de daño 3 turnos
    FlechaPerforante,    // Arquera: ATK*1.6 ignora DEF
    Red,                 // Arquera: enemigo -1 PA durante 2 turnos
    MarcaCazador,        // Arquera: enemigo recibe +50% de daño 3 turnos
    DetenerTiempo,       // Halley: todos los enemigos -2 PA
    Paradoja,            // Halley: repite gratis la última habilidad
    Sentencia,           // Halley: daño según la vida que le falta al enemigo
    LanzaLuz,            // Nato: ATK*2.0 ignora mitad DEF
    Bendicion,           // Nato: regenera vida y +DEF 3 turnos
    Exilio,              // Nato: enemigo pierde sus turnos durante 2 turnos
    Supernova,           // Nato: ATK*2.0 a todos ignora DEF
    Genesis,             // Nato: revive con 50% de vida (una vez por combate)
    MundoNuevo,          // Nato: cura 25%, quita efectos negativos y +2 PA
};

inline constexpr int kArtEffectCount = static_cast<int>(ArtEffect::MundoNuevo) + 1;

// Rangos de habilidad (sala de entrenamiento): I base, II +20% efecto,
// III +40% efecto y -25% de coste de maná.
inline constexpr int kMaxArtRank = 3;
inline int artRankBonusPct(int rank)      { return rank >= 3 ? 40 : rank == 2 ? 20 : 0; }
inline int artRankRequiredLevel(int rank) { return rank >= 3 ? 20 : rank == 2 ? 10 : 1; }
inline const char* artRankRoman(int rank) { return rank >= 3 ? "III" : rank == 2 ? "II" : "I"; }

struct Art {
    std::string name;
    int         apCost;
    int         manaCost;     // ya con el descuento del rango III
    ArtEffect   effect;
    std::string description;
    int         rank = 1;

    // Nombre con el rango cuando está mejorada: "Bola de Fuego II"
    std::string label() const {
        return rank > 1 ? name + " " + artRankRoman(rank) : name;
    }
};
