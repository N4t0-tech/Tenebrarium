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
    int tier = 1;    // Rareza de armas y armaduras: 1 Común … 4 Legendario
};

inline constexpr int kMaxItemTier = 4;

inline const char* tierName(int tier) {
    switch (tier) {
        case 2:  return "Raro";
        case 3:  return "Épico";
        case 4:  return "Legendario";
        default: return "Común";
    }
}
