#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include "systems/game_system.hpp"
#include "conductor.hpp"

extern conductor g_conductor;

class physics_system : public game_system {
    public:
    void update(float delta_time);
};

#endif