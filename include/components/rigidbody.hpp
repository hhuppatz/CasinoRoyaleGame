#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <SFML/Graphics/RectangleShape.hpp>

struct rigidbody
{
    float velocity[2];
    float Mass;
    sf::RectangleShape Hitbox;
    bool can_collide;
    float base_size[2]; // Base dimensions of the hitbox (before scaling)
};

#endif