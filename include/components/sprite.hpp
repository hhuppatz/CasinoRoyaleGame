#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <optional>

struct sprite {
    std::optional<sf::Sprite> sprite_obj;
    sf::Texture texture;
    std::string texture_name;
    
    // Default constructor to make sprite default-constructible
    sprite() = default;
    
    // Constructor for when we actually have a sprite
    sprite(sf::Sprite s, sf::Texture t, std::string name) 
        : sprite_obj(s), texture(t), texture_name(name) {}
};