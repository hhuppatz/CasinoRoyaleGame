#pragma once

#include "../entity.hpp"
#include "../conductor.hpp"
#include "game_system.hpp"
#include <SFML/Graphics/Rect.hpp>

extern conductor g_conductor;

class item_system : public game_system {
    public:
    void update(float dt);
    entity check_collision(sf::FloatRect hitbox); // Return collided entity
    bool can_be_picked_up(entity item_entity); // Check if item can be picked up
    void pickup(entity item_entity); // pickup item entity
    void drop(entity item_entity, float position_x, float position_y, float velocity_x, float velocity_y); // drop item entity
};