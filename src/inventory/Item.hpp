#pragma once

#include <string>

enum class ItemType {
    Weapon,      // statBonus = ATK bonus
    Armor,       // statBonus = DEF bonus
    Consumable,  // statBonus = HP recuperado al usar
};

struct Item {
    std::string name;
    std::string description;
    ItemType type;
    int value;       // precio base en monedas de oro (la tienda puede añadir margen)
    int slots;       // slots de inventario que ocupa (normalmente 1)
    int statBonus;   // modificador genérico: +ATK si arma, +DEF si armadura, +HP si consumible
};
