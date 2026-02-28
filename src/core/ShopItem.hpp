#pragma once
#include "inventory/Item.hpp"

struct ShopItem {
    Item item;
    int  price;
    bool sold;
};
