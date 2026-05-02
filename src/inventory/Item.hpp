#pragma once

#include <string>

enum class ItemType {
    Weapon,
    Armor,
    Consumable,
    Bomb,
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
