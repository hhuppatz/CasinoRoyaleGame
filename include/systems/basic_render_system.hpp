#ifndef BASIC_RENDER_SYSTEM_H
#define BASIC_RENDER_SYSTEM_H

#include "systems/game_system.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include "components/sprite.hpp"
#include <SFML/Graphics/View.hpp>
#include "conductor.hpp"

extern conductor g_conductor;

class basic_render_system : public game_system {
    public:
    void update(sf::RenderWindow& window);
};

#endif