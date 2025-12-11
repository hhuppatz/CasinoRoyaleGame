#pragma once

#include "game_system.hpp"
#include "../conductor.hpp"
#include "inventory_system.hpp"
#include "item_system.hpp"

extern conductor g_conductor;

class player_input_system : public game_system {
    public:
    void update(inventory_system& inventory_sys, item_system& item_sys, bool space_was_pressed);
    void reset();
};