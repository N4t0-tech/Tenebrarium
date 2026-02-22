#include "Inventory.hpp"
#include <algorithm>
#include <numeric>

Inventory::Inventory(int totalSlots)
    : totalSlots_(totalSlots)
{}

bool Inventory::addItem(const Item& item) {
    if (freeSlots() < item.slots) {
        return false;
    }
    items_.push_back(item);
    return true;
}

bool Inventory::removeItem(const std::string& itemName) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&](const Item& i) { return i.name == itemName; });

    if (it == items_.end()) return false;
    items_.erase(it);
    return true;
}

int Inventory::usedSlots() const {
    return std::accumulate(items_.begin(), items_.end(), 0,
        [](int sum, const Item& i) { return sum + i.slots; });
}
