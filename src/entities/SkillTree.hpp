#pragma once

// SkillTree — definición de árboles de habilidades por clase.
// Cada clase tiene hasta 4 tiers (niveles 5, 10, 15, 20).
// Cada tier ofrece 1 o 2 opciones; el jugador elige una.
// Lo aprendido se suma a las 3 habilidades base; el jugador equipa 3 de ellas.

#include "combat/Art.hpp"
#include <string>
#include <vector>

// Forward declaration de PlayerClass (definido en Player.hpp)
enum class PlayerClass;

struct SkillChoice {
    ArtEffect   effect;
    std::string name;
    std::string desc;    // descripción mostrada en el árbol
    int         apCost;
    int         manaCost;
};

struct SkillTier {
    int                     requiredLevel; // 5, 10, 15, 20
    std::vector<SkillChoice> choices;      // 1 opción = obligatoria, 2 = elegir
};

struct SkillTreeDef {
    PlayerClass             cls;
    std::vector<SkillTier>  tiers;
};

// Retorna el árbol de habilidades para la clase dada.
// Definido en SkillTree.cpp.
const SkillTreeDef& getSkillTree(PlayerClass cls);

// Las 3 habilidades con las que empieza cada clase.
const std::vector<SkillChoice>& getBaseSkills(PlayerClass cls);

// Habilidad base o del árbol de la clase con ese efecto; nullptr si no existe.
const SkillChoice* findClassSkill(PlayerClass cls, ArtEffect effect);
