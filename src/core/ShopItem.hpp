#pragma once
#include "inventory/Item.hpp"

// Entrada del stock de la tienda. El precio puede diferir de item.value
// (se le suma un margen por piso en generateShopStock).
// sold == true significa que el ítem ya fue comprado; se muestra tachado
// pero permanece en la lista para mantener el layout de la UI estable.
struct ShopItem {
    Item item;
    int  price;
    bool sold;
};
