#include "systems/inventory_system.hpp"
#include "components/rigidbody.hpp"
#include "systems/item_system.hpp"
#include "components/inventory.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include "components/entity_state.hpp"
#include "components/item.hpp"

void inventory_system::attempt_pickups(item_system& item_sys) {
    for (auto ent : entities) {
        auto& entity_state_comp = g_conductor.get_component<entity_state>(ent);
        if (!entity_state_comp.is_active) {
            continue;
        }
        auto& inventory_comp = g_conductor.get_component<inventory>(ent);
        auto& rigidbody_comp = g_conductor.get_component<rigidbody>(ent);
        entity collision = item_sys.check_collision(sf::FloatRect(rigidbody_comp.Hitbox.getPosition(), rigidbody_comp.Hitbox.getSize()));
        if (collision < MAX_ENTITIES) { // Valid item entity found
            // Check if the item can actually be picked up
            if (!item_sys.can_be_picked_up(collision)) {
                continue;
            }
            
            // Safety check: verify the entity has an item component before accessing it
            if (!g_conductor.has_component<item>(collision)) {
                std::cerr << "WARNING: Entity " << collision << " found by collision detection but has no item component!" << std::endl;
                continue;
            }
            
            if (g_conductor.get_component<item>(collision).is_coin) {
                inventory_comp.coins++;
                g_conductor.destroy_entity(collision); // Now coin is stored as int in inventory, can remove from game worldcompletely
            } else {
                inventory_comp.items.push_back(collision); // Add item entity to inventory
                item_sys.pickup(collision); // Set item entity to be picked up
            }
        }
    }
}

void inventory_system::drop(item_system& item_sys, entity ent, int slot) {
    auto& inventory_comp = g_conductor.get_component<inventory>(ent);
    auto& rigidbody_comp = g_conductor.get_component<rigidbody>(ent);
    auto& item_entity = inventory_comp.items[slot];
    inventory_comp.items.erase(inventory_comp.items.begin() + slot); // Remove item entity from inventory
    item_sys.drop(item_entity, rigidbody_comp.Hitbox.getGeometricCenter().x, rigidbody_comp.Hitbox.getGeometricCenter().y, 0, 500); // Drop item entity with a velocity of 0, 500
}

// To draw a single inventory to the UI
void inventory_system::draw_ui(sf::RenderWindow& window, entity player_entity) {
    auto& inventory_comp = g_conductor.get_component<inventory>(player_entity);
    for (size_t i = 0; i < inventory_comp.items.size(); i++) {
        auto& item_entity = inventory_comp.items[i];
        auto& item_comp = g_conductor.get_component<item>(item_entity);
        item_comp.ui_view->setPosition({static_cast<float>(300 * i), 1080.f - 200.f});
        window.draw(item_comp.ui_view.value());
    }
}