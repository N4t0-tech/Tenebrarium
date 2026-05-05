#pragma once

#include <string>

enum class ItemType {
    Weapon,      // statBonus = ATK bonus
    Armor,       // statBonus = DEF bonus
    Consumable,  // statBonus = HP recuperado al usar
    Bomb,        // Cantidad apilada, usa quantity
};

struct Item {
    std::string name;
    std::string description;
    ItemType type;
    int value;       // Valor en monedas
    int slots;       // Slots que ocupa en inventario (default 1)
    int statBonus;   // Bono de ataque/defensa/HP según tipo
    int quantity;    // Cantidad apilada (default 1, para pociones/bombas)
};
