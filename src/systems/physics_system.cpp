#include "systems/physics_system.hpp"
#include "components/rigidbody.hpp"
#include "components/gravity.hpp"
#include "components/transform.hpp"
#include <algorithm>

constexpr float MAX_VELOCITY = 3000.0f;

void physics_system::update(float delta_time) {
    for (auto entity : entities) {
        auto& transform1 = g_conductor.get_component<transform>(entity);
        auto& rigidbody1 = g_conductor.get_component<rigidbody>(entity);
        auto& gravity1 = g_conductor.get_component<gravity>(entity);

        rigidbody1.velocity[1] += (gravity1.force * delta_time) / rigidbody1.Mass;

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