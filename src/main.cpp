#include "component_serialization.hpp"
#include "components/entity_state.hpp"
#include "components/gravity.hpp"
#include "components/inventory.hpp"
#include "components/item.hpp"
#include "components/jump.hpp"
#include "components/network.hpp"
#include "components/player.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "conductor.hpp"
#include "entity.hpp"
#include "network_manager.hpp"
#include "systems/basic_render_system.hpp"
#include "systems/collision_detection_system.hpp"
#include "systems/inventory_system.hpp"
#include "systems/item_system.hpp"
#include "systems/jump_system.hpp"
#include "systems/network_system.hpp"
#include "systems/physics_system.hpp"
#include "systems/player_input_system.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <steam/steamnetworkingtypes.h>
#include <string>
#include <thread>
#include <vector>

// Define the global conductor instance before including test_system.h
// to avoid name shadowing issues
conductor g_conductor;

constexpr float GRAVITY = 7000.0f;

void create_coin(network_system &network_system1,
                 const sf::Texture &coin_texture,
                 const std::string &coin_texture_name,
                 const sf::Texture &coin_ui_texture,
                 const std::string &coin_ui_texture_name);

void register_components() {
    g_conductor.register_component<transform>();
    g_conductor.register_component<player>();
    g_conductor.register_component<sprite>();
    g_conductor.register_component<gravity>();
    g_conductor.register_component<rigidbody>();
    g_conductor.register_component<jump>();
    g_conductor.register_component<inventory>();
    g_conductor.register_component<item>();
    g_conductor.register_component<entity_state>();
    g_conductor.register_component<network>();
}

void register_signatures() {
    // Set the signature for the test system (uses transform, player, and camera
    // components)
    signature player_input_system_signature;
    player_input_system_signature.set(g_conductor.get_component_type<player>(),
                                      true);
    player_input_system_signature.set(
        g_conductor.get_component_type<rigidbody>(), true);
    g_conductor.set_system_signature<player_input_system>(
        player_input_system_signature);

    // Set the signature for the basic render system (uses transform and sprite
    // components)
    signature basic_render_system_signature;
    basic_render_system_signature.set(
        g_conductor.get_component_type<transform>(), true);
    basic_render_system_signature.set(g_conductor.get_component_type<sprite>(),
                                      true);
    basic_render_system_signature.set(
        g_conductor.get_component_type<entity_state>(), true);
    g_conductor.set_system_signature<basic_render_system>(
        basic_render_system_signature);

    // Set the signature for the collision detection system (uses transform and
    // rigidbody components)
    signature collision_detection_system_signature;
    collision_detection_system_signature.set(
        g_conductor.get_component_type<transform>(), true);
    collision_detection_system_signature.set(
        g_conductor.get_component_type<rigidbody>(), true);
    collision_detection_system_signature.set(
        g_conductor.get_component_type<entity_state>(), true);
    g_conductor.set_system_signature<collision_detection_system>(
        collision_detection_system_signature);

    // Set the signature for the physics system (uses rigidbody and gravity
    // components)
    signature physics_system_signature;
    physics_system_signature.set(g_conductor.get_component_type<rigidbody>(),
                                 true);
    physics_system_signature.set(g_conductor.get_component_type<gravity>(),
                                 true);
    physics_system_signature.set(g_conductor.get_component_type<entity_state>(),
                                 true);
    g_conductor.set_system_signature<physics_system>(physics_system_signature);

    // Set the signature for the jump system (uses jump component)
    signature jump_system_signature;
    jump_system_signature.set(g_conductor.get_component_type<jump>(), true);
    jump_system_signature.set(g_conductor.get_component_type<entity_state>(),
                              true);
    g_conductor.set_system_signature<jump_system>(jump_system_signature);

    // Set the signature for the inventory system (uses inventory component)
    signature inventory_system_signature;
    inventory_system_signature.set(g_conductor.get_component_type<inventory>(),
                                   true);
    inventory_system_signature.set(g_conductor.get_component_type<rigidbody>(),
                                   true);
    inventory_system_signature.set(
        g_conductor.get_component_type<entity_state>(), true);
    g_conductor.set_system_signature<inventory_system>(
        inventory_system_signature);

    // Set the signature for the item system (uses item component)
    signature item_system_signature;
    item_system_signature.set(g_conductor.get_component_type<item>(), true);
    item_system_signature.set(g_conductor.get_component_type<rigidbody>(),
                              true);
    item_system_signature.set(g_conductor.get_component_type<transform>(),
                              true);
    item_system_signature.set(g_conductor.get_component_type<entity_state>(),
                              true);
    g_conductor.set_system_signature<item_system>(item_system_signature);

    // Set the signature for the network system (uses network component)
    signature network_system_signature;
    network_system_signature.set(g_conductor.get_component_type<network>(),
                                 true);
    g_conductor.set_system_signature<network_system>(network_system_signature);
}

// Forward declaration for component serialization initialization
void InitializeComponentSerializers();

int main() {
    NetworkManager::Get().Init(); // Init networking

    g_conductor.init(); // Must be called before using conductor

    // Initialize component serializers
    InitializeComponentSerializers();

    register_components();

    auto player_input_system1 =
        g_conductor.register_system<player_input_system>();
    auto basic_render_system1 =
        g_conductor.register_system<basic_render_system>();
    auto collision_detection_system1 =
        g_conductor.register_system<collision_detection_system>();
    auto physics_system1 = g_conductor.register_system<physics_system>();
    auto jump_system1 = g_conductor.register_system<jump_system>();
    auto inventory_system1 = g_conductor.register_system<inventory_system>();
    auto item_system1 = g_conductor.register_system<item_system>();
    auto network_system1 = g_conductor.register_system<network_system>();

    register_signatures();

    // Wire up network packet callback
    NetworkManager::Get().SetPacketCallback(
        [&network_system1](HSteamNetConnection conn, const void *data,
                           size_t size) {
            network_system1->handle_packet(conn, data, size);
        });

    // Camera view (independent of player entity existence)
    sf::View view(sf::FloatRect({0.f, 0.f}, {1920.0f, 1080.0f}));

    // Load player texture once (will be reused for all player entities)
    auto player_texture_name = "assets/images/player.png";
    sf::Texture player_texture;
    if (!player_texture.loadFromFile(player_texture_name)) {
        std::cerr << "Error loading player texture" << std::endl;
        return 1;
    }

    // Load coin textures once (will be reused for all coin entities)
    auto coin_texture_name = "assets/images/coin.png";
    sf::Texture coin_texture;
    if (!coin_texture.loadFromFile(coin_texture_name)) {
        std::cerr << "Error loading coin texture" << std::endl;
        return 1;
    }

    auto coin_ui_texture_name = "assets/images/giantpoopycoin.png";
    sf::Texture coin_ui_texture;
    if (!coin_ui_texture.loadFromFile(coin_ui_texture_name)) {
        std::cerr << "Error loading coin UI texture" << std::endl;
        return 1;
    }

    // Create a ground entity with a transform component and sprite component
    auto ground = g_conductor.create_entity();
    g_conductor.add_component<transform>(
        ground, transform{{0.0f, 100.0f}, {0.0f, 100.0f}, {1.0f, 1.0f}});
    g_conductor.add_component<rigidbody>(
        ground, rigidbody{{0.0f, 0.0f},
                          2000.0f,
                          sf::RectangleShape({1280.0f, 32.0f}),
                          true,
                          {1280.0f, 32.0f}});

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
    g_conductor.add_component<entity_state>(ground, entity_state{true, false});

    // Game state
    enum class GameState { Menu,
                           Playing };
    GameState current_state = GameState::Menu;

    // Networked player state
    bool awaiting_player_network_id = false;
    bool awaiting_coin_network_id = false;
    std::optional<entity> local_player =
        std::nullopt; // Will be set once networked player is created

    // Create a window
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}),
                                   "Casino Royale - Multiplayer");

    // Set framerate limit to 144
    window.setFramerateLimit(144);

    // Create a clock to measure the elapsed time between frames
    sf::Clock clock;

    // Create a graphical text to display the player position and velocity
    const sf::Font font("assets/fonts/arial.ttf");
    sf::Text pos_text(font, "Player Position: 0, 0", 50);
    sf::Text vel_text(font, "Player Velocity: 0, 0", 50);
    sf::Text coins_text(font, "Coins: 0", 50);

    // Menu Text
    sf::Text menu_text(font, "Press H to Host\nPress J to Join (localhost)",
                       50);
    menu_text.setPosition({500, 400});

    // Create a rectangle shape to display the inventory slots
    sf::RectangleShape inventory_slot(sf::Vector2f(200.f, 200.f));
    inventory_slot.setFillColor(sf::Color::White);
    inventory_slot.setPosition({0.f, 1080.f - 200.f});

    bool has_focus = true;

    bool space_is_pressed = false;
    bool space_was_pressed = false;
    bool h_is_pressed = false;
    bool j_is_pressed = false;
    bool c_is_pressed = false;
    bool c_was_pressed = false;

    // Main loop
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::FocusGained>()) {
                has_focus = true;
            }
            if (event->is<sf::Event::FocusLost>()) {
                has_focus = false;
            }
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.033f);

        if (window.hasFocus()) {
            space_was_pressed = space_is_pressed;
            space_is_pressed =
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
            h_is_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H);
            j_is_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
            c_was_pressed = c_is_pressed;
            c_is_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
        }

        NetworkManager::Get().Update(); // Process network events

        window.clear(sf::Color::Blue);

        switch (current_state) {

        case GameState::Menu:
            window.draw(menu_text);

            if (h_is_pressed) {
                if (NetworkManager::Get().StartHost(27020)) {
                    current_state = GameState::Playing;
                    std::cout << "Hosting on port 27020" << std::endl;

                    // Host allocates its own network ID directly
                    uint32_t host_id =
                        NetworkManager::Get().AllocateNetworkId();
                    network_system1
                        ->clear_pending_granted_id(); // Make sure it's clear
                    // Manually set the pending granted ID for the host
                    // We'll need a helper method for this, or we can create
                    // player immediately

                    // Create host player entity immediately
                    std::cout << "Creating host player with ID: " << host_id
                              << std::endl;
                    entity player_entity =
                        g_conductor.create_networked_entity(host_id, true);
                    local_player = player_entity;

                    // Add all player components
                    g_conductor.add_component<transform>(
                        player_entity,
                        transform{{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}});
                    g_conductor.add_component<player>(player_entity, player{});
                    g_conductor.add_component<rigidbody>(
                        player_entity,
                        rigidbody{{0.0f, 0.0f},
                                  100.0f,
                                  sf::RectangleShape({32.0f, 48.0f}),
                                  true,
                                  {32.0f, 48.0f}});
                    g_conductor.add_component<gravity>(player_entity,
                                                       gravity{GRAVITY});
                    g_conductor.add_component<jump>(player_entity,
                                                    jump{-1000.0f, false});

                    // Add sprite component
                    sprite player_sprite_comp;
                    player_sprite_comp.texture = player_texture;
                    player_sprite_comp.texture_name = player_texture_name;
                    player_sprite_comp.sprite_obj =
                        sf::Sprite(player_sprite_comp.texture);
                    g_conductor.add_component<sprite>(player_entity,
                                                      player_sprite_comp);

                    g_conductor.add_component<inventory>(
                        player_entity,
                        inventory{0, std::vector<entity>(), 0, 3});
                    g_conductor.add_component<entity_state>(
                        player_entity, entity_state{true, false});

                    // Configure network component - specify which components to sync continuously
                    auto &net_comp = g_conductor.get_component<network>(player_entity);
                    net_comp.networked_components = {ComponentID::Transform, ComponentID::Rigidbody};

                    // Broadcast entity init to clients (when they join)
                    network_system1->send_entity_init(player_entity);
                    std::cout << "Host player created!" << std::endl;
                }
            } else if (j_is_pressed) {
                if (NetworkManager::Get().Connect("127.0.0.1:27020")) {
                    current_state = GameState::Playing;
                    std::cout << "Joining localhost..." << std::endl;

                    // Client requests network ID from host
                    network_system1->request_network_id();
                    awaiting_player_network_id = true;
                }
            }
            break;

        case GameState::Playing:
            // PLAYING STATE

            // Check if we're waiting for network ID grant and create player
            // when granted
            if (awaiting_player_network_id &&
                network_system1->has_pending_granted_id()) {
                uint32_t granted_id = network_system1->get_pending_granted_id();
                network_system1->clear_pending_granted_id();
                awaiting_player_network_id = false;

                std::cout << "Creating networked player with ID: " << granted_id
                          << std::endl;

                try {
                    // Create networked player entity
                    std::cout << "  - Creating entity..." << std::endl;
                    entity player_entity =
                        g_conductor.create_networked_entity(granted_id, true);
                    local_player = player_entity;

                    // Add transform component
                    std::cout << "  - Adding transform..." << std::endl;
                    g_conductor.add_component<transform>(
                        player_entity,
                        transform{{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}});

                    // Add player marker component
                    std::cout << "  - Adding player..." << std::endl;
                    g_conductor.add_component<player>(player_entity, player{});

                    // Add rigidbody component
                    std::cout << "  - Adding rigidbody..." << std::endl;
                    g_conductor.add_component<rigidbody>(
                        player_entity,
                        rigidbody{{0.0f, 0.0f},
                                  100.0f,
                                  sf::RectangleShape({32.0f, 48.0f}),
                                  true,
                                  {32.0f, 48.0f}});

                    // Add gravity component
                    std::cout << "  - Adding gravity..." << std::endl;
                    g_conductor.add_component<gravity>(player_entity,
                                                       gravity{GRAVITY});

                    // Add jump component
                    std::cout << "  - Adding jump..." << std::endl;
                    g_conductor.add_component<jump>(player_entity,
                                                    jump{-1000.0f, false});

                    // Add sprite component
                    std::cout << "  - Adding sprite..." << std::endl;
                    sprite player_sprite_comp;
                    player_sprite_comp.texture = player_texture;
                    player_sprite_comp.texture_name = player_texture_name;
                    player_sprite_comp.sprite_obj =
                        sf::Sprite(player_sprite_comp.texture);
                    g_conductor.add_component<sprite>(player_entity,
                                                      player_sprite_comp);

                    // Add inventory component
                    std::cout << "  - Adding inventory..." << std::endl;
                    g_conductor.add_component<inventory>(
                        player_entity,
                        inventory{0, std::vector<entity>(), 0, 3});

                    // Add entity state component
                    std::cout << "  - Adding entity_state..." << std::endl;
                    g_conductor.add_component<entity_state>(
                        player_entity, entity_state{true, false});

                    // Configure network component - specify which components to sync continuously
                    std::cout << "  - Configuring network sync..." << std::endl;
                    auto &net_comp = g_conductor.get_component<network>(player_entity);
                    net_comp.networked_components = {ComponentID::Transform, ComponentID::Rigidbody};

                    // Send entity initialization to network
                    std::cout << "  - Sending entity init to network..."
                              << std::endl;
                    network_system1->send_entity_init(player_entity);

                    std::cout << "Networked player created and broadcasted!"
                              << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "ERROR creating player entity: " << e.what()
                              << std::endl;
                } catch (...) {
                    std::cerr << "UNKNOWN ERROR creating player entity!"
                              << std::endl;
                }
            }

            // Only process game logic if local player exists
            if (!local_player.has_value()) {
                // Show waiting message
                window.setView(window.getDefaultView());
                sf::Text waiting_text(font, "Waiting for network ID...", 50);
                waiting_text.setPosition({700, 500});
                window.draw(waiting_text);
                window.display();
                continue;
            }

            // Only create coin on key press (not hold) - debounced input
            if (c_is_pressed && !c_was_pressed && !awaiting_coin_network_id) {
                // Request network ID for the coin
                if (NetworkManager::Get().IsHost()) {
                    // Host can create immediately
                    create_coin(*network_system1, coin_texture, coin_texture_name,
                                coin_ui_texture, coin_ui_texture_name);
                } else {
                    // Client must request ID first
                    network_system1->request_network_id();
                    awaiting_coin_network_id = true;
                    std::cout << "Requesting network ID for coin..." << std::endl;
                }
            }

            // Check if we're waiting for coin network ID grant
            if (awaiting_coin_network_id &&
                network_system1->has_pending_granted_id()) {
                uint32_t granted_id = network_system1->get_pending_granted_id();
                network_system1->clear_pending_granted_id();
                awaiting_coin_network_id = false;

                std::cout << "Received network ID " << granted_id << " for coin, creating..." << std::endl;

                // Create the networked entity with the granted ID
                auto item_entity = g_conductor.create_networked_entity(granted_id, true);

                // Add transform component
                g_conductor.add_component<transform>(
                    item_entity,
                    transform{{200.0f, -200.0f}, {200.0f, -200.0f}, {1.0f, 1.0f}});

                // Add rigidbody component
                g_conductor.add_component<rigidbody>(
                    item_entity, rigidbody{{0.0f, -200.0f},
                                           20.0f,
                                           sf::RectangleShape({8.0f, 8.0f}),
                                           true,
                                           {8.0f, 8.0f}});

                // Add gravity component
                g_conductor.add_component<gravity>(item_entity, gravity{GRAVITY});

                // Create sprite for world rendering using persistent texture reference
                sprite item_sprite_comp;
                item_sprite_comp.texture = coin_texture;
                item_sprite_comp.texture_name = coin_texture_name;
                item_sprite_comp.sprite_obj = sf::Sprite(item_sprite_comp.texture);
                g_conductor.add_component<sprite>(item_entity, item_sprite_comp);

                // Create UI view sprite using persistent texture reference
                sprite item_ui_view_sprite;
                item_ui_view_sprite.texture = coin_ui_texture;
                item_ui_view_sprite.texture_name = coin_ui_texture_name;
                item_ui_view_sprite.sprite_obj = sf::Sprite(item_ui_view_sprite.texture);

                // Add item component with UI sprite
                g_conductor.add_component<item>(
                    item_entity,
                    item{item_ui_view_sprite.sprite_obj.value(), false, 0, -1, true});

                // Add entity state component
                g_conductor.add_component<entity_state>(item_entity,
                                                        entity_state{true, false});

                // Configure network component
                auto &net_comp = g_conductor.get_component<network>(item_entity);
                net_comp.networked_components = {ComponentID::Transform, ComponentID::Rigidbody, ComponentID::Item, ComponentID::Sprite};

                // Send entity init to host (who will broadcast to others)
                network_system1->send_entity_init(item_entity);

                std::cout << "Created and sent coin entity with network ID: " << granted_id << std::endl;
            }

            entity player_entity = local_player.value();

            if (has_focus) {
                player_input_system1->update(
                    *inventory_system1, *item_system1,
                    space_was_pressed); // Process player input
            }

            physics_system1->update(dt); // Simulate physics and forces

            item_system1->update(dt);
            inventory_system1->attempt_pickups(*item_system1);

            collision_detection_system1->update(
                *jump_system1); // Run collision detection and resolve
                                // collisions

            // Network System - send/receive network updates
            network_system1->update(dt);

            // Update camera view BEFORE rendering so rendering uses the correct
            // view
            view.setCenter({g_conductor.get_component<transform>(player_entity)
                                .position[0],
                            g_conductor.get_component<transform>(player_entity)
                                .position[1]}); // Camera follows player
            window.setView(view);

            basic_render_system1->update(window);

            /*******************************************
             *                                         *
             *               STATIC UI                 *
             *                                         *
             *******************************************/

            sf::View worldView = window.getView();   // Save for later
            window.setView(window.getDefaultView()); // Set view to default to
                                                     // draw UI elements

            pos_text.setString(
                "Player Position: " +
                std::to_string(
                    g_conductor.get_component<transform>(player_entity)
                        .position[0]) +
                ", " +
                std::to_string(
                    g_conductor.get_component<transform>(player_entity)
                        .position[1]));
            pos_text.setPosition({10, 10});
            window.draw(pos_text);

            vel_text.setString(
                "Player Velocity: " +
                std::to_string(
                    g_conductor.get_component<rigidbody>(player_entity)
                        .velocity[0]) +
                ", " +
                std::to_string(
                    g_conductor.get_component<rigidbody>(player_entity)
                        .velocity[1]));
            vel_text.setPosition({10, 50});
            window.draw(vel_text);

            coins_text.setString(
                "Coins: " +
                std::to_string(
                    g_conductor.get_component<inventory>(player_entity).coins));
            coins_text.setPosition(
                {1920.f - coins_text.getLocalBounds().size.x - 30.f, 10.f});
            window.draw(coins_text);

            inventory_slot.setPosition({0.f, 1080.f - 200.f});
            window.draw(inventory_slot);

            inventory_slot.setPosition({300.f, 1080.f - 200.f});
            window.draw(inventory_slot);

            inventory_slot.setPosition({600.f, 1080.f - 200.f});
            window.draw(inventory_slot);

            inventory_system1->draw_ui(window,
                                       player_entity); // Draw inventory UI

            player_input_system1->reset(); // Reset player for next frame

            window.setView(worldView); // Set view back to world view
        }

        window.display();
    }

    return 0;
}

// Create a coin entity with persistent textures (passed from main)
// NOTE: This should only be called by the HOST. Clients handle coin creation
// in the main loop after receiving a granted network ID.
void create_coin(network_system &network_system1,
                 const sf::Texture &coin_texture,
                 const std::string &coin_texture_name,
                 const sf::Texture &coin_ui_texture,
                 const std::string &coin_ui_texture_name) {
    // Host allocates its own network ID directly
    uint32_t network_id = NetworkManager::Get().AllocateNetworkId();

    // Create the networked entity (this already adds the network component)
    auto item_entity = g_conductor.create_networked_entity(network_id, true);

    // Add transform component
    g_conductor.add_component<transform>(
        item_entity,
        transform{{200.0f, -200.0f}, {200.0f, -200.0f}, {1.0f, 1.0f}});

    // Add rigidbody component
    g_conductor.add_component<rigidbody>(
        item_entity, rigidbody{{0.0f, -200.0f},
                               20.0f,
                               sf::RectangleShape({8.0f, 8.0f}),
                               true,
                               {8.0f, 8.0f}});

    // Add gravity component
    g_conductor.add_component<gravity>(item_entity, gravity{GRAVITY});

    // Create sprite for world rendering using persistent texture reference
    sprite item_sprite_comp;
    item_sprite_comp.texture = coin_texture;
    item_sprite_comp.texture_name = coin_texture_name;
    item_sprite_comp.sprite_obj = sf::Sprite(item_sprite_comp.texture);
    g_conductor.add_component<sprite>(item_entity, item_sprite_comp);

    // Create UI view sprite using persistent texture reference
    sprite item_ui_view_sprite;
    item_ui_view_sprite.texture = coin_ui_texture;
    item_ui_view_sprite.texture_name = coin_ui_texture_name;
    item_ui_view_sprite.sprite_obj = sf::Sprite(item_ui_view_sprite.texture);

    // Add item component with UI sprite
    g_conductor.add_component<item>(
        item_entity,
        item{item_ui_view_sprite.sprite_obj.value(), false, 0, -1, true});

    // Add entity state component
    g_conductor.add_component<entity_state>(item_entity,
                                            entity_state{true, false});

    // Configure network component (already added by create_networked_entity)
    // Specify which components should be synced over the network
    auto &net_comp = g_conductor.get_component<network>(item_entity);
    net_comp.networked_components = {ComponentID::Transform, ComponentID::Rigidbody, ComponentID::Item, ComponentID::Sprite};

    std::cout << "Created coin entity with network ID: " << network_id << std::endl;

    // Broadcast the entity to all clients (if host)
    if (NetworkManager::Get().IsHost()) {
        network_system1.send_entity_init(item_entity);
        std::cout << "Broadcasted coin entity to all clients" << std::endl;
    }
}