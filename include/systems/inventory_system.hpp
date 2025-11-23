#pragma once

#include "../entity.hpp"
#include "../conductor.hpp"
#include "game_system.hpp"

extern conductor g_conductor;

class inventory_system : public game_system {
    public:
    void update();
    void pickup(entity item); // store item entity in inventory
    void drop(entity item); // remove item entity from inventory
};