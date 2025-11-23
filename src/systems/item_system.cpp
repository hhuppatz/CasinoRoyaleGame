#include "systems/item_system.hpp"
#include "components/item.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include <SFML/Graphics/Rect.hpp>

bool rectanglesIntersect(float x1, float y1, float w1, float h1,
    float x2, float y2, float w2, float h2) {
    return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
}

void item_system::update(float dt) {
    for (auto entity : entities) {
        auto& item_comp = g_conductor.get_component<item>(entity);
        if (item_comp.active) {
            if (item_comp.time_until_pickup > 0) {
                item_comp.time_until_pickup -= dt;    
            }
            if (item_comp.time_until_despawn > 0) {
                item_comp.time_until_despawn -= dt;
                // If item has despawned, set active to false
                if (item_comp.time_until_despawn <= 0) {
                    item_comp.active = false;
                }
            }
        }
    }
}

// Check if item entity can be picked up, return true if can be picked up
bool item_system::can_be_picked_up(entity item_entity) {
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    return item_comp.active && item_comp.time_until_pickup <= 0;
}

// Check if given hitbox intersects with any item entity, return collided entity or -1 if no collision
// Returns first item entity that intersects with given hitbox only
entity item_system::check_collision(sf::FloatRect hitbox) {
    for (auto entity : entities) {
        auto& item_comp = g_conductor.get_component<item>(entity);
        auto& transform_comp = g_conductor.get_component<transform>(entity);
        auto& rigidbody_comp = g_conductor.get_component<rigidbody>(entity);
        if (item_comp.active) {
            if (rectanglesIntersect(transform_comp.position[0], transform_comp.position[1], rigidbody_comp.Hitbox.getSize().x, rigidbody_comp.Hitbox.getSize().y, hitbox.position.x, hitbox.position.y, hitbox.size.x, hitbox.size.y)) {
                return entity; // Collision found, return entity
            }
        }
    }
    return -1; // No collision found
}

// Pickup item entity, set active to false
void item_system::pickup(entity item_entity) {
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    item_comp.active = false;
}

// Drop item entity at given position with given velocity, set active to true
void item_system::drop(entity item_entity, float position_x, float position_y, float velocity_x, float velocity_y) {
    auto& item_comp = g_conductor.get_component<item>(item_entity);
    auto& transform_comp = g_conductor.get_component<transform>(item_entity);
    auto& rigidbody_comp = g_conductor.get_component<rigidbody>(item_entity);
    transform_comp.position[0] = position_x;
    transform_comp.position[1] = position_y;
    rigidbody_comp.velocity[0] = velocity_x;
    rigidbody_comp.velocity[1] = velocity_y;
    item_comp.active = true;
}

