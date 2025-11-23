#ifndef JUMP_SYSTEM_HPP
#define JUMP_SYSTEM_HPP

#include "systems/game_system.hpp"
#include "conductor.hpp"

class jump_system : public game_system {
    public:
    void reset_jump(entity e);
};

#endif