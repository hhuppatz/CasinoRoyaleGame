#include "systems/item_system.hpp"
#include "components/item.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "help_functions.hpp"
#include <SFML/Graphics/Rect.hpp>
#include "components/entity_state.hpp"
#include <iostream>

void item_system::update(float dt) {
    for (auto entity : entities) {
        auto& item_comp = g_conductor.get_component<item>(entity);
        auto& entity_state_comp = g_conductor.get_component<entity_state>(entity);
        if (entity_state_comp.is_active) {
            if (item_comp.time_until_pickup > 0) {
                item_comp.time_until_pickup -= dt;    
            }
            if (item_comp.time_until_despawn > 0) {
                item_comp.time_until_despawn -= dt;
                // If item has despawned, set active to false
                if (item_comp.time_until_despawn <= 0) {
                    entity_state_comp.is_active = false;
                }
            }
        }
    }
}

// Check if item entity can be picked up, return true if can be picked up
bool item_system::can_be_picked_up(entity item_entity) {
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    auto& entity_state_comp = g_conductor.get_component<entity_state>(item_entity);
    return entity_state_comp.is_active && item_comp.time_until_pickup <= 0 && !item_comp.is_picked_up;
}

// Check if given hitbox intersects with any item entity, return collided entity or MAX_ENTITIES if no collision
// Returns first item entity that intersects with given hitbox only
entity item_system::check_collision(sf::FloatRect hitbox) {
    for (auto entity : entities) {
        // Safety check: verify entity has all required components
        if (!g_conductor.has_component<transform>(entity) ||
            !g_conductor.has_component<rigidbody>(entity) ||
            !g_conductor.has_component<entity_state>(entity) ||
            !g_conductor.has_component<item>(entity)) {
            std::cerr << "ERROR: Entity " << entity << " in item_system but missing required components!" << std::endl;
            std::cerr << "  Has transform: " << g_conductor.has_component<transform>(entity) << std::endl;
            std::cerr << "  Has rigidbody: " << g_conductor.has_component<rigidbody>(entity) << std::endl;
            std::cerr << "  Has entity_state: " << g_conductor.has_component<entity_state>(entity) << std::endl;
            std::cerr << "  Has item: " << g_conductor.has_component<item>(entity) << std::endl;
            continue;
        }
        
        auto& transform_comp = g_conductor.get_component<transform>(entity);
        auto& rigidbody_comp = g_conductor.get_component<rigidbody>(entity);
        auto& entity_state_comp = g_conductor.get_component<entity_state>(entity);
        if (entity_state_comp.is_active) {
            if (rectanglesIntersect(transform_comp.position[0], transform_comp.position[1], rigidbody_comp.Hitbox.getSize().x, rigidbody_comp.Hitbox.getSize().y, hitbox.position.x, hitbox.position.y, hitbox.size.x, hitbox.size.y)) {
                return entity; // Collision found, return entity
            }
        }
    }
    return MAX_ENTITIES; // No collision found - return invalid entity ID
}

// Pickup item entity, set active to false
void item_system::pickup(entity item_entity) {
    auto& entity_state_comp = g_conductor.get_component<entity_state>(item_entity);
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    entity_state_comp.is_active = false;
    item_comp.is_picked_up = true;
}

// Drop item entity at given position with given velocity, set active to true
void item_system::drop(entity item_entity, float position_x, float position_y, float velocity_x, float velocity_y) {
    auto& transform_comp = g_conductor.get_component<transform>(item_entity);
    auto& rigidbody_comp = g_conductor.get_component<rigidbody>(item_entity);
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    auto& entity_state_comp = g_conductor.get_component<entity_state>(item_entity);
    item_comp.time_until_pickup = 3; // 3 seconds until item can be picked up again
    transform_comp.position[0] = position_x;
    transform_comp.position[1] = position_y;
    rigidbody_comp.velocity[0] = velocity_x;
    rigidbody_comp.velocity[1] = velocity_y;
    entity_state_comp.is_active = true;
    item_comp.is_picked_up = false;
}

