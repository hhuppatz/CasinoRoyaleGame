#ifndef ITEM_HPP
#define ITEM_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <optional>

struct item {
    std::optional<sf::Sprite> ui_view;
    bool is_picked_up;
    float time_until_pickup; // Set to negative to never pickup (?)
    float time_until_despawn; // Set to negative to never despawn
    bool is_coin; // Set to true if the item is a coin

    // Default constructor to make item default-constructible
    item() = default;
    
     // Constructor for when we actually have a full item with ui view
     item(sf::Sprite s, bool is_picked_up, float time_until_pickup, float time_until_despawn, bool is_coin = false) 
         : ui_view(s), is_picked_up(is_picked_up), time_until_pickup(time_until_pickup), time_until_despawn(time_until_despawn), is_coin(is_coin) {}
};

#endif