#include "SkillTree.hpp"
#include "Player.hpp"

// ─── Árboles de habilidades por clase ────────────────────────────────────────
// Cada árbol tiene 4 tiers: Lv5, Lv10, Lv15, Lv20, alternando 1 → 2 → 1 → 2.
// Lv5 es siempre la habilidad de mapa de la clase ([Q] hechizo / [T] trampa),
// obligatoria. Tiers con 2 opciones: el jugador elige una.

static const SkillTreeDef kWarriorTree = {
    PlayerClass::Warrior,
    {
        { 5, {
            { ArtEffect::GritoDeGuerra,  "GritoG",    "+ATK 2t / aturde cerca [Q]", 1, 10 },
        }},
        { 10, {
            { ArtEffect::GolpeDemoledor, "GolpeD",    "ATK*1.4 ignora mitad DEF", 2, 0 },
            { ArtEffect::EscudoTotal,    "EscTotal",  "-70% próximo golpe",       1, 5 },
        }},
        { 15, {
            { ArtEffect::BerserkWarrior, "Berserk",   "+50%ATK -30%DEF 3t",       1, 15 },
        }},
        { 20, {
            { ArtEffect::Fortaleza,      "Fortalza",  "Regen 10%HP 3t",           1, 15 },
            { ArtEffect::GolpeDemoledor, "Terremto",  "ATK*1.2 a todos",          2, 20 },
        }},
    }
};

static const SkillTreeDef kPicaroTree = kWarriorTree; // placeholder

static const SkillTreeDef kMageTree = {
    PlayerClass::Mage,
    {
        { 5, {
            { ArtEffect::BolaDeFuego,   "BolFugo",  "ATK+10 a TODOS [Q]",        2, 20 },
        }},
        { 10, {
            { ArtEffect::Congelar,      "Congelar", "Enemigo -1PA [Q]",          1, 10 },
            { ArtEffect::Drenaje,       "Drenaje",  "Daño + roba HP [Q]",        3, 30 },
        }},
        { 15, {
            { ArtEffect::Rayo,          "Rayo",     "ATK*1.8 ignora DEF [Q]",    2, 25 },
        }},
        { 20, {
            { ArtEffect::TormentaArcana,"TormArca", "ATK/2 todos+hielo",         2, 30 },
            { ArtEffect::EscudoArcano,  "EscArcno", "Absorbe 1 golpe",           1, 20 },
        }},
    }
};

static const SkillTreeDef kNecrofagoTree = kMageTree; // placeholder

static const SkillTreeDef kRangerTree = {
    PlayerClass::Ranger,
    {
        { 5, {
            { ArtEffect::Trampa,        "Trampa",   "Retardo + trampas mapa [T]", 1, 0 },
        }},
        { 10, {
            { ArtEffect::DisparoDoble,  "Disp2x",   "2 ataques rápidos",         2, 0 },
            { ArtEffect::Veneno,        "Veneno",   "DoT ATK/3 3t",              1, 0 },
        }},
        { 15, {
            { ArtEffect::GolpeCritico,  "GolpCri",  "ATK*2.5",                   2, 0 },
        }},
        { 20, {
            { ArtEffect::LluviaFlechas, "LluvFle",  "3xATK*0.6 aleatorio",       2, 10 },
            { ArtEffect::Sombra,        "Sombra",   "Esquiva+contraataca",       1, 10 },
        }},
    }
};

static const SkillTreeDef kBallesteroTree = kRangerTree; // placeholder

static const SkillTreeDef kHalleyTree = {
    PlayerClass::Halley,
    {
        { 5, {
            { ArtEffect::LluviaEstelar, "LluvEst",  "ATK*1.5 a todos [Q]",       3, 25 },
        }},
        { 10, {
            { ArtEffect::Creacion,      "Creación", "Recupera 50%HP",            1, 20 },
            { ArtEffect::PresenciaDivina,"PresDivn", "+ATK/2+5 3t",              2, 25 },
        }},
        { 15, {
            { ArtEffect::AbsorbVida,    "AbsVida",  "Roba 30%HP enemigo",        2, 20 },
        }},
        { 20, {
            { ArtEffect::CampoEquilibrio,"CampoEq", "Iguala HP (promedio)",      2, 25 },
            { ArtEffect::LluviaEstelar, "LluvEst2", "ATK*2.0 a todos (máx)",     3, 18 },
        }},
    }
};

static const SkillTreeDef kNatoTree = {
    PlayerClass::Nato,
    {
        { 5, {
            { ArtEffect::DistorsionTemporal, "DistTemp", "Enemigo pierde PA [Q]",  2, 20 },
        }},
        { 10, {
            { ArtEffect::VinculoEterno,      "VincEtrn", "Daño+cura 150%",         1, 10 },
            { ArtEffect::JuicioFinal,        "JuicioF",  "ATK*2.0 ignora DEF",     3, 30 },
        }},
        { 15, {
            { ArtEffect::ColapsoDimensional, "ColaDim",  "ATK*1.5 -stats enemigo", 2, 25 },
        }},
        { 20, {
            { ArtEffect::DistorsionTemporal, "DistTmp2", "todos enemigos -2PA",    2, 20 },
            { ArtEffect::JuicioFinal,        "JuicioF2", "ATK*3.0 ignora DEF (máx)", 3, 20 },
        }},
    }
};

const SkillTreeDef& getSkillTree(PlayerClass cls) {
    switch (cls) {
        case PlayerClass::Warrior:    return kWarriorTree;
        case PlayerClass::Picaro:     return kPicaroTree;
        case PlayerClass::Mage:       return kMageTree;
        case PlayerClass::Necrofago:  return kNecrofagoTree;
        case PlayerClass::Ranger:     return kRangerTree;
        case PlayerClass::Ballestero: return kBallesteroTree;
        case PlayerClass::Halley:     return kHalleyTree;
        case PlayerClass::Nato:       return kNatoTree;
    }
    return kWarriorTree;
}
