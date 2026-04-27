#pragma once

#include <string>

// Clase base para todas las entidades del juego (jugador, enemigos, NPCs).
// takeDamage aplica la DEF de la entidad; takeDamageRaw la ignora (daño mágico, DoT, etc.).
class Entity {
public:
    Entity(const std::string& name, int maxHp, int attack, int defense);
    virtual ~Entity() = default;

    const std::string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getAttack() const { return attack_; }
    int getDefense() const { return defense_; }

    bool isAlive() const { return hp_ > 0; }

    void takeDamage(int amount);
    void takeDamageRaw(int amount);   // ignora DEF — usar para DoT, magia, trampas
    void heal(int amount);

protected:
    std::string name_;
    int hp_;
    int maxHp_;
    int attack_;
    int defense_;
};
