#ifndef COLLISION_DETECTION_SYSTEM_HPP
#define COLLISION_DETECTION_SYSTEM_HPP

#include "systems/game_system.hpp"
#include "conductor.hpp"

extern conductor g_conductor;

class jump_system;

class collision_detection_system : public game_system {
    public:
    void update(jump_system& jump_system);
};

#endif