#include <set>
#include "entity.hpp"

#ifndef GAME_SYSTEM_H
#define GAME_SYSTEM_H

class game_system {
    public:
        std::set<entity> entities;
};

#endif