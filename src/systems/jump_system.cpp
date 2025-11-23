#include "systems/jump_system.hpp"
#include "components/jump.hpp"
#include <algorithm>

extern conductor g_conductor;

void jump_system::reset_jump(entity e) {
    if (std::find(entities.begin(), entities.end(), e) == entities.end()) return;
        g_conductor.get_component<jump>(e).is_jumping = false;
}