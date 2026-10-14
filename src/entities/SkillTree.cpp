#include "SkillTree.hpp"
#include "Player.hpp"

// ─── Habilidades base y árboles por clase ────────────────────────────────────
// Cada clase empieza con 3 habilidades base. El árbol tiene 4 tiers (Lv5, Lv10,
// Lv15, Lv20) alternando 1 → 2 → 1 → 2 opciones; ninguna repite una base.
// Lo aprendido no se equipa solo: el jugador elige sus 3 habilidades con [H].

static const std::vector<SkillChoice> kWarriorBase = {
    { ArtEffect::GolpeDemoledor, "Golpe Demoledor", "ATK*1.4, ignora mitad DEF",  2, 0  },
    { ArtEffect::GritoDeGuerra,  "Grito de Guerra", "+ATK por 2 turnos",          1, 10 },
    { ArtEffect::EscudoTotal,    "Escudo Total",    "-70% al próximo golpe",      1, 5  },
};
static const std::vector<SkillChoice> kMageBase = {
    { ArtEffect::BolaDeFuego, "Bola de Fuego", "ATK+10 a todos [Q]",           2, 20 },
    { ArtEffect::Congelar,    "Congelar",      "Enemigo -1 PA [Q]",            1, 10 },
    { ArtEffect::Drenaje,     "Drenaje",       "Daño y curas lo mismo [Q]",    3, 30 },
};
static const std::vector<SkillChoice> kRangerBase = {
    { ArtEffect::DisparoDoble, "Disparo Doble", "2 disparos al objetivo",      2, 0 },
    { ArtEffect::Trampa,       "Trampa",        "Daño retardado [T]",          1, 0 },
    { ArtEffect::Veneno,       "Veneno",        "ATK/3 por turno, 3 turnos",   1, 0 },
};
static const std::vector<SkillChoice> kHalleyBase = {
    { ArtEffect::DistorsionTemporal, "Distorsión Temporal", "Enemigo pierde sus PA [Q]", 2, 20 },
    { ArtEffect::VinculoEterno,      "Vínculo Eterno",      "Daño y curas 150%",         1, 10 },
    { ArtEffect::JuicioFinal,        "Juicio Final",        "ATK*2.0, ignora DEF",       3, 30 },
};
static const std::vector<SkillChoice> kNatoBase = {
    { ArtEffect::LluviaEstelar,   "Juicio Estelar",   "ATK*1.5 a todos [Q]",   3, 25 },
    { ArtEffect::Creacion,        "Creación",         "Recupera 50% de vida",  1, 20 },
    { ArtEffect::PresenciaDivina, "Presencia Divina", "+ATK por 3 turnos",     2, 25 },
};

static const SkillTreeDef kWarriorTree = {
    PlayerClass::Warrior,
    {
        { 5, {
            { ArtEffect::BerserkWarrior, "Furia Berserker", "+50% ATK, -30% DEF 3t",     1, 15 },
        }},
        { 10, {
            { ArtEffect::Fortaleza,      "Fortaleza",       "Regenera 10% vida 3t",      1, 15 },
            { ArtEffect::Terremoto,      "Terremoto",       "ATK*1.2 a todos",           2, 25 },
        }},
        { 15, {
            { ArtEffect::Ejecutar,       "Ejecutar",        "ATK*1.5, x3 si <30% vida",  2, 20 },
        }},
        { 20, {
            { ArtEffect::Rompecorazas,   "Rompecorazas",    "Daño y -50% DEF 3t",        1, 20 },
            { ArtEffect::UltimoBastion,  "Último Bastión",  "No bajas de 1 HP 2t",       1, 30 },
        }},
    }
};

static const SkillTreeDef kMageTree = {
    PlayerClass::Mage,
    {
        { 5, {
            { ArtEffect::Rayo,           "Rayo",            "ATK*1.8, ignora DEF [Q]",   2, 25 },
        }},
        { 10, {
            { ArtEffect::TormentaArcana, "Tormenta Arcana", "ATK/2 a todos y congela",   2, 30 },
            { ArtEffect::EscudoArcano,   "Escudo Arcano",   "Absorbe el próximo golpe",  1, 20 },
        }},
        { 15, {
            { ArtEffect::CadenaRayos,    "Cadena de Rayos", "Salta a hasta 3 enemigos",  2, 35 },
        }},
        { 20, {
            { ArtEffect::Meteoro,        "Meteoro",         "ATK*2.2 a todos",           3, 50 },
            { ArtEffect::Maldicion,      "Maldición",       "Recibe +30% daño 3t",       1, 20 },
        }},
    }
};

static const SkillTreeDef kRangerTree = {
    PlayerClass::Ranger,
    {
        { 5, {
            { ArtEffect::FlechaPerforante, "Flecha Perforante", "ATK*1.6, ignora DEF",     2, 10 },
        }},
        { 10, {
            { ArtEffect::GolpeCritico,     "Tiro Certero",      "ATK*2.5",                 2, 10 },
            { ArtEffect::Red,              "Red",               "Enemigo -1 PA 2t",        1, 10 },
        }},
        { 15, {
            { ArtEffect::LluviaFlechas,    "Lluvia de Flechas", "3 flechas al azar",       2, 15 },
        }},
        { 20, {
            { ArtEffect::MarcaCazador,     "Marca del Cazador", "Recibe +50% daño 3t",     1, 10 },
            { ArtEffect::Sombra,           "Paso Sombrío",      "Esquiva y contraataca",   1, 15 },
        }},
    }
};

static const SkillTreeDef kHalleyTree = {
    PlayerClass::Halley,
    {
        { 5, {
            { ArtEffect::AbsorbVida,         "Absorber Vida",       "Roba 30% de su vida",       2, 20 },
        }},
        { 10, {
            { ArtEffect::CampoEquilibrio,    "Campo de Equilibrio", "Iguala vuestras vidas",     2, 25 },
            { ArtEffect::ColapsoDimensional, "Colapso Dimensional", "Daño, -30% ATK/DEF 3t",     2, 25 },
        }},
        { 15, {
            { ArtEffect::DetenerTiempo,      "Detener el Tiempo",   "Todos los enemigos -2 PA",  2, 35 },
        }},
        { 20, {
            { ArtEffect::Paradoja,           "Paradoja",            "Repite gratis la última",   1, 30 },
            { ArtEffect::Sentencia,          "Sentencia",           "Daño por la vida perdida",  2, 30 },
        }},
    }
};

static const SkillTreeDef kNatoTree = {
    PlayerClass::Nato,
    {
        { 5, {
            { ArtEffect::LanzaLuz,   "Lanza de Luz", "ATK*2.0, ignora mitad DEF",  2, 20 },
        }},
        { 10, {
            { ArtEffect::Bendicion,  "Bendición",    "Regenera y +DEF 3t",         1, 25 },
            { ArtEffect::Exilio,     "Exilio",       "Enemigo sin turnos 2t",      2, 30 },
        }},
        { 15, {
            { ArtEffect::Supernova,  "Supernova",    "ATK*2.0 a todos, ignora DEF", 3, 45 },
        }},
        { 20, {
            { ArtEffect::Genesis,    "Génesis",      "Revives con 50% (1 vez)",    1, 40 },
            { ArtEffect::MundoNuevo, "Mundo Nuevo",  "Cura 25%, limpia, +2 PA",    1, 35 },
        }},
    }
};

const SkillTreeDef& getSkillTree(PlayerClass cls) {
    switch (cls) {
        case PlayerClass::Warrior:    return kWarriorTree;
        case PlayerClass::Mage:       return kMageTree;
        case PlayerClass::Ranger:     return kRangerTree;
        case PlayerClass::Halley:     return kHalleyTree;
        case PlayerClass::Nato:       return kNatoTree;
    }
    return kWarriorTree;
}

const std::vector<SkillChoice>& getBaseSkills(PlayerClass cls) {
    switch (cls) {
        case PlayerClass::Warrior:    return kWarriorBase;
        case PlayerClass::Mage:       return kMageBase;
        case PlayerClass::Ranger:     return kRangerBase;
        case PlayerClass::Halley:     return kHalleyBase;
        case PlayerClass::Nato:       return kNatoBase;
    }
    return kWarriorBase;
}

const SkillChoice* findClassSkill(PlayerClass cls, ArtEffect effect) {
    for (const auto& s : getBaseSkills(cls))
        if (s.effect == effect) return &s;
    for (const auto& tier : getSkillTree(cls).tiers)
        for (const auto& c : tier.choices)
            if (c.effect == effect) return &c;
    return nullptr;
}
