#ifndef PLAYER_INPUT_SYSTEM_HPP
#define PLAYER_INPUT_SYSTEM_HPP

#include "systems/game_system.hpp"
#include "conductor.hpp"

extern conductor g_conductor;

class player_input_system : public game_system {
    public:
    void update(bool space_was_pressed);
    void reset();
};

#endif