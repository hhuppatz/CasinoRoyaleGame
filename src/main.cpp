#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <iostream>
#include <optional>
#include <string>
#include "conductor.hpp"
#include "components/transform.hpp"
#include "components/player.hpp"
#include "components/circle.hpp"
#include "components/gravity.hpp"
#include "components/rigidbody.hpp"
#include "components/jump.hpp"

#include "systems/jump_system.hpp"
#include "systems/player_input_system.hpp"
#include "systems/basic_render_system.hpp"
#include "systems/collision_detection_system.hpp"
#include "systems/physics_system.hpp"

// Define the global conductor instance before including test_system.h
// to avoid name shadowing issues
conductor g_conductor;

void register_components() {
    g_conductor.register_component<transform>();
    g_conductor.register_component<player>();
    g_conductor.register_component<sprite>();
    g_conductor.register_component<gravity>();
    g_conductor.register_component<rigidbody>();
    g_conductor.register_component<jump>();
}

void register_signatures() {
    // Set the signature for the test system (uses transform, player, and camera components)
    signature player_input_system_signature;
    player_input_system_signature.set(g_conductor.get_component_type<player>(), true);
    player_input_system_signature.set(g_conductor.get_component_type<rigidbody>(), true);
    g_conductor.set_system_signature<player_input_system>(player_input_system_signature);

    // Set the signature for the basic render system (uses transform and sprite components)
    signature basic_render_system_signature;
    basic_render_system_signature.set(g_conductor.get_component_type<transform>(), true);
    basic_render_system_signature.set(g_conductor.get_component_type<sprite>(), true);
    g_conductor.set_system_signature<basic_render_system>(basic_render_system_signature);

    // Set the signature for the collision detection system (uses transform and rigidbody components)
    signature collision_detection_system_signature;
    collision_detection_system_signature.set(g_conductor.get_component_type<transform>(), true);
    collision_detection_system_signature.set(g_conductor.get_component_type<rigidbody>(), true);
    g_conductor.set_system_signature<collision_detection_system>(collision_detection_system_signature);

    // Set the signature for the physics system (uses rigidbody and gravity components)
    signature physics_system_signature;
    physics_system_signature.set(g_conductor.get_component_type<rigidbody>(), true);
    physics_system_signature.set(g_conductor.get_component_type<gravity>(), true);
    g_conductor.set_system_signature<physics_system>(physics_system_signature);

    // Set the signature for the jump system (uses jump component)
    signature jump_system_signature;
    jump_system_signature.set(g_conductor.get_component_type<jump>(), true);
    g_conductor.set_system_signature<jump_system>(jump_system_signature);
}

int main()
{
    g_conductor.init(); // Must be called before using conductor
    register_components();
    
    auto player_input_system1 = g_conductor.register_system<player_input_system>();
    auto basic_render_system1 = g_conductor.register_system<basic_render_system>();
    auto collision_detection_system1 = g_conductor.register_system<collision_detection_system>();
    auto physics_system1 = g_conductor.register_system<physics_system>();
    auto jump_system1 = g_conductor.register_system<jump_system>();

    register_signatures();

    // Create player entity with a transform component and player component
    auto player_entity = g_conductor.create_entity();
    g_conductor.add_component<transform>(player_entity, transform{{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}});
    g_conductor.add_component<player>(player_entity, player{});
    g_conductor.add_component<rigidbody>(player_entity, rigidbody{{0.0f, 0.0f}, 1.0f, sf::RectangleShape({32.0f, 48.0f}), {32.0f, 48.0f}});
    g_conductor.add_component<gravity>(player_entity, gravity{9000.0f});
    g_conductor.add_component<jump>(player_entity, jump{-1000.0f, false});

    // Create a camera for the player entity
    sf::View view(sf::FloatRect({0.f, 0.f}, {1920.0f, 1080.0f}));

    // Create a texture and sprite for the player entity
    auto player_texture_name = "assets/images/player.png";
    sf::Texture texture;
    if (!texture.loadFromFile(player_texture_name)) {
        std::cerr << "Error loading texture" << std::endl;
        return 1;
    }
    // Create sprite component with texture first, then create sprite from component's texture
    sprite player_sprite_comp;
    player_sprite_comp.texture = texture;
    player_sprite_comp.texture_name = player_texture_name;
    player_sprite_comp.sprite_obj = sf::Sprite(player_sprite_comp.texture);
    g_conductor.add_component<sprite>(player_entity, player_sprite_comp);

    // Create a second entity (ground) with a transform component and sprite component
    auto ground = g_conductor.create_entity();
    g_conductor.add_component<transform>(ground, transform{{0.0f, 100.0f}, {0.0f, 100.0f}, {1.0f, 1.0f}});
    g_conductor.add_component<rigidbody>(ground, rigidbody{{0.0f, 0.0f}, 1.0f, sf::RectangleShape({1280.0f, 32.0f}), {1280.0f, 32.0f}});
    // Create sprite component with texture first, then create sprite from component's texture
    auto ground_texture_name = "assets/images/big_ground.png";
    sf::Texture ground_texture;
    if (!ground_texture.loadFromFile(ground_texture_name)) {
        std::cerr << "Error loading texture" << std::endl;
        return 1;
    }
    sprite ground_sprite_comp;
    ground_sprite_comp.texture = ground_texture;
    ground_sprite_comp.texture_name = ground_texture_name;
    ground_sprite_comp.sprite_obj = sf::Sprite(ground_sprite_comp.texture);
    g_conductor.add_component<sprite>(ground, ground_sprite_comp);

    // Create a window
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "CMake SFML Project");
    window.setFramerateLimit(144);

    sf::Clock clock;

    // Create a graphical text to display
    const sf::Font font("assets/fonts/arial.ttf");
    sf::Text pos_text(font, "Player Position: 0, 0", 50);
    sf::Text vel_text(font, "Player Velocity: 0, 0", 50);

    bool space_was_pressed = false;

    // Main loop
    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        window.clear(sf::Color::Blue);

        // Check space key state at the start of the frame
        bool space_currently_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
        player_input_system1->update(space_was_pressed); // Process player input
        space_was_pressed = space_currently_pressed; // Update for next frame
        physics_system1->update(dt); // Simulate physics and forces
        collision_detection_system1->update(*jump_system1); // Run collision detection and resolve collisions

        // Update camera view BEFORE rendering so rendering uses the correct view
        view.setCenter({g_conductor.get_component<transform>(player_entity).position[0], g_conductor.get_component<transform>(player_entity).position[1]}); // Camera follows player
        window.setView(view);
        
        basic_render_system1->update(window);

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        pos_text.setString("Player Position: " + std::to_string(g_conductor.get_component<transform>(player_entity).position[0]) + ", " + std::to_string(g_conductor.get_component<transform>(player_entity).position[1]));
        pos_text.setPosition({10, 10});
        window.draw(pos_text);

        vel_text.setString("Player Velocity: " + std::to_string(g_conductor.get_component<rigidbody>(player_entity).velocity[0]) + ", " + std::to_string(g_conductor.get_component<rigidbody>(player_entity).velocity[1]));
        vel_text.setPosition({10, 50});
        window.draw(vel_text);

        player_input_system1->reset(); // Reset player for next frame
        window.display();
    }

    return 0;
}