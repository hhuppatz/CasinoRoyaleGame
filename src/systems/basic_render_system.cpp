#include "systems/basic_render_system.hpp"
#include "components/transform.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include "components/entity_state.hpp"
#include "systems/inventory_system.hpp"

void basic_render_system::update(sf::RenderWindow& window) {
    for (auto entity : entities) {
        auto& entity_state_comp = g_conductor.get_component<entity_state>(entity);
        if (!entity_state_comp.is_active) {
            continue;
        }
        auto& sprite1 = g_conductor.get_component<sprite>(entity);
        auto& transform1 = g_conductor.get_component<transform>(entity);
        if (sprite1.sprite_obj.has_value()) {
            sprite1.sprite_obj->setPosition({transform1.position[0], transform1.position[1]});
            sprite1.sprite_obj->setScale({transform1.scale[0], transform1.scale[1]});
            sprite1.sprite_obj->setTexture(sprite1.texture);
            window.draw(*sprite1.sprite_obj);
        }
    }
}