#include "systems/player_input_system.hpp"
#include "components/rigidbody.hpp"
#include "components/jump.hpp"
#include <SFML/Window/Keyboard.hpp>

extern conductor g_conductor;

void player_input_system::update(bool space_was_pressed) {
    for (auto entity : entities) {
        // Player controls to move player rigidbody velocity, to integrate with collision detection system
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            g_conductor.get_component<rigidbody>(entity).velocity[0] += 240.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            g_conductor.get_component<rigidbody>(entity).velocity[0] -= 240.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) &&
            !g_conductor.get_component<jump>(entity).is_jumping &&
            space_was_pressed == false)
        {
            g_conductor.get_component<rigidbody>(entity).velocity[1] = g_conductor.get_component<jump>(entity).initial_velocity;
            g_conductor.get_component<jump>(entity).is_jumping = true;
        }
    }
}

void player_input_system::reset() {
    for (auto entity : entities) {
        g_conductor.get_component<rigidbody>(entity).velocity[0] = 0.f;
    }
}