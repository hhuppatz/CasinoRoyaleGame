#pragma once

#include <vector>
#include "../entity.hpp"

struct inventory {
    int coins = 0; // coins in inventory
    std::vector<entity> items; // stored items
    int selected_slot;
    int max_items; // max items in inventory

    // Default constructor to make inventory default-constructible
    inventory() = default;

    // Constructor for when we have a full inventory
    inventory(int coins, std::vector<entity> items, int selected_slot, int max_items) 
        : coins(coins), items(items), selected_slot(selected_slot), max_items(max_items) {}
};