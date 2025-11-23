#pragma once

#include <vector>
#include "../entity.hpp"

struct inventory {
    std::vector<entity> items; // stored items
    int max_items; // max items in inventory
};