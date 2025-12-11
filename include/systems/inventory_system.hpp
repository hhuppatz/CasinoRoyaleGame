#pragma once

#include "../conductor.hpp"
#include "../entity.hpp"
#include "game_system.hpp"
#include "item_system.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

extern conductor g_conductor;

class inventory_system : public game_system {
public:
  void update(item_system &item_sys);
  void attempt_pickups(item_system &item_sys); // store item entity in inventory
  void drop(item_system &item_sys, entity ent,
            int slot); // remove item entity from inventory
  void draw_ui(sf::RenderWindow &window, entity player_entity);
};