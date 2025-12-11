#include "systems/player_input_system.hpp"
#include "components/entity_state.hpp"
#include "components/inventory.hpp"
#include "components/jump.hpp"
#include "components/network.hpp"
#include "components/rigidbody.hpp"
#include "systems/inventory_system.hpp"
#include "systems/item_system.hpp"
#include <SFML/Window/Keyboard.hpp>

extern conductor g_conductor;

void player_input_system::update(inventory_system &inventory_sys,
                                 item_system &item_sys,
                                 bool space_was_pressed) {
  for (auto entity : entities) {
    auto &entity_state_comp = g_conductor.get_component<entity_state>(entity);
    if (!entity_state_comp.is_active) {
      continue;
    }
    
    // Only process input for LOCAL players (not remote networked players)
    if (g_conductor.has_component<network>(entity)) {
      auto &network_comp = g_conductor.get_component<network>(entity);
      if (!network_comp.is_local) {
        continue; // Skip remote players
      }
    }
    // Player controls to move player rigidbody velocity, to integrate with
    // collision detection system
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
      g_conductor.get_component<rigidbody>(entity).velocity[0] += 240.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
      g_conductor.get_component<rigidbody>(entity).velocity[0] -= 240.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) &&
        !g_conductor.get_component<jump>(entity).is_jumping &&
        space_was_pressed == false) {
      g_conductor.get_component<rigidbody>(entity).velocity[1] =
          g_conductor.get_component<jump>(entity).initial_velocity;
      g_conductor.get_component<jump>(entity).is_jumping = true;
    }

    auto &inventory_comp = g_conductor.get_component<inventory>(entity);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num1)) {
      inventory_comp.selected_slot = 0;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num2)) {
      inventory_comp.selected_slot = 1;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Num3)) {
      inventory_comp.selected_slot = 2;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
      inventory_sys.drop(item_sys, entity, inventory_comp.selected_slot);
    }
  }
}

void player_input_system::reset() {
  for (auto entity : entities) {
    // Only reset LOCAL players (not remote networked players)
    if (g_conductor.has_component<network>(entity)) {
      auto &network_comp = g_conductor.get_component<network>(entity);
      if (!network_comp.is_local) {
        continue; // Skip remote players
      }
    }
    g_conductor.get_component<rigidbody>(entity).velocity[0] = 0.f;
  }
}