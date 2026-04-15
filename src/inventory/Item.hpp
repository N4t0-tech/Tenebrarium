#pragma once

#include <string>

enum class ItemType {
    Weapon,
    Armor,
    Consumable,
};

struct Item {
    std::string name;
    std::string description;
    ItemType type;
    int value;       // gold value
    int slots;       // inventory slots consumed (default 1)
    int statBonus;   // generic stat modifier (attack, defense, or hp depending on type)
};
