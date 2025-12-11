#include "systems/physics_system.hpp"
#include "components/rigidbody.hpp"
#include "components/gravity.hpp"
#include "components/transform.hpp"
#include "components/network.hpp"
#include <algorithm>
#include "components/entity_state.hpp"

constexpr float MAX_VELOCITY = 3000.0f;

void physics_system::update(float delta_time) {
    for (auto entity : entities) {
        auto& entity_state_comp = g_conductor.get_component<entity_state>(entity);
        if (!entity_state_comp.is_active) {
            continue;
        }
        
        // Only apply physics to LOCAL entities (not remote networked entities)
        if (g_conductor.has_component<network>(entity)) {
            auto& network_comp = g_conductor.get_component<network>(entity);
            if (!network_comp.is_local) {
                continue; // Skip remote entities - their physics is simulated on the owner's machine
            }
        }
        auto& transform1 = g_conductor.get_component<transform>(entity);
        auto& rigidbody1 = g_conductor.get_component<rigidbody>(entity);
        auto& gravity1 = g_conductor.get_component<gravity>(entity);

        rigidbody1.velocity[1] += gravity1.force * delta_time / 3;

        // Clamp velocity to prevent infinite velocity
        rigidbody1.velocity[1] = std::clamp(rigidbody1.velocity[1], -MAX_VELOCITY, MAX_VELOCITY);
        rigidbody1.velocity[0] = std::clamp(rigidbody1.velocity[0], -MAX_VELOCITY, MAX_VELOCITY);

        // Save last position
        transform1.last_position[0] = transform1.position[0];
        transform1.last_position[1] = transform1.position[1];

        // Update current position based on velocity
        transform1.position[0] += rigidbody1.velocity[0] * delta_time;
        transform1.position[1] += rigidbody1.velocity[1] * delta_time;
    }
}