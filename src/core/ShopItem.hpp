#pragma once
#include "inventory/Item.hpp"

// Entrada del stock de la tienda. El precio es por unidad y puede diferir de
// item.value (se le suma un margen por piso en generateShopStock).
// item.quantity son las unidades que quedan en la tienda.
// sold == true significa que se agotó; se muestra tachado
// pero permanece en la lista para mantener el layout de la UI estable.
struct ShopItem {
    Item item;
    int  price;
    bool sold;
};
