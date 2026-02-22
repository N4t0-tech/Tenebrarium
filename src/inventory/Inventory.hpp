#pragma once

#include "Item.hpp"
#include <vector>
#include <optional>

// Slot-based inventory. Each item occupies Item::slots consecutive slots.
class Inventory {
public:
    explicit Inventory(int totalSlots);

    bool addItem(const Item& item);
    bool removeItem(const std::string& itemName);

    int usedSlots() const;
    int totalSlots() const { return totalSlots_; }
    int freeSlots() const { return totalSlots_ - usedSlots(); }

    const std::vector<Item>& items() const { return items_; }

private:
    int totalSlots_;
    std::vector<Item> items_;
};
