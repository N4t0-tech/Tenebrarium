#pragma once

#include "Item.hpp"
#include <vector>
#include <optional>

// Slot-based inventory. Each item occupies Item::slots consecutive slots.
class Inventory {
public:
    Inventory(int totalSlots);
    bool addItem(const Item& item);
    bool removeItem(const std::string& itemName);
    int usedSlots() const;
    int freeSlots() const { return totalSlots_ - usedSlots(); }
    int totalSlots() const { return totalSlots_; }
    const std::vector<Item>& items() const { return items_; }
    void setTotalSlots(int s) { totalSlots_ = s; }
    int getItemCount(const std::string& itemName) const;
private:
    int totalSlots_;
    std::vector<Item> items_;
};
