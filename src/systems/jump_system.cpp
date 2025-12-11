#include "systems/jump_system.hpp"
#include "components/jump.hpp"
#include <algorithm>
#include "components/entity_state.hpp"

extern conductor g_conductor;

void jump_system::reset_jump(entity e) {
    auto& entity_state_comp = g_conductor.get_component<entity_state>(e);
    if (!entity_state_comp.is_active) {
        return;
    }
    if (std::find(entities.begin(), entities.end(), e) == entities.end()) return;
    g_conductor.get_component<jump>(e).is_jumping = false;
}