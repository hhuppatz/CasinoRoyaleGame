#include "systems/network_system.hpp"
#include "component_serialization.hpp"
#include "components/entity_state.hpp"
#include "components/gravity.hpp"
#include "components/inventory.hpp"
#include "components/item.hpp"
#include "components/jump.hpp"
#include "components/network.hpp"
#include "components/player.hpp"
#include "components/rigidbody.hpp"
#include "components/sprite.hpp"
#include "components/transform.hpp"
#include "conductor.hpp"
#include "entity.hpp"
#include "network_manager.hpp"
#include "packets.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <steam/steamnetworkingtypes.h>
#include <vector>

extern conductor g_conductor;

void network_system::update(float dt) {
    NetworkManager &nm = NetworkManager::Get();
    if (!nm.IsConnected())
        return;

    if (nm.IsHost()) {
        broadcast_state();
        broadcast_component_updates();
    } else {
        send_input();
        send_component_updates();
        update_remote_entities(dt);
    }
}

void network_system::handle_packet(HSteamNetConnection conn, const void *data,
                                   size_t size) {
    if (size < sizeof(PacketHeader))
        return;

    const PacketHeader *header = static_cast<const PacketHeader *>(data);

    switch (header->type) {
    case PacketType::ReserveNetworkIDRequest:
        handle_reserve_id_request(conn, data, size);
        break;
    case PacketType::NetworkIDReserved:
        handle_network_id_reserved(data, size);
        break;
    case PacketType::NetworkIDGranted:
        handle_network_id_granted(data, size);
        break;
    case PacketType::EntityInitPacket:
        handle_entity_init(data, size);
        break;
    case PacketType::ComponentBatchUpdate:
        handle_component_batch_update(data, size);
        break;
    case PacketType::OwnershipTransferPacket:
        handle_ownership_transfer(data, size);
        break;
    case PacketType::PlayerInput: {
        if (size < sizeof(PlayerInputPacket))
            return;
        // Handle player input from clients (host only)
        break;
    }
    case PacketType::GameStateUpdate: {
        if (size < sizeof(GameStateUpdatePacket))
            return;
        const GameStateUpdatePacket *packet =
            static_cast<const GameStateUpdatePacket *>(data);

        const uint8_t *ptr =
            static_cast<const uint8_t *>(data) + sizeof(GameStateUpdatePacket);
        for (uint32_t i = 0; i < packet->entity_count; ++i) {
            if (ptr + sizeof(EntityStateData) >
                static_cast<const uint8_t *>(data) + size)
                break;
            const EntityStateData *state =
                reinterpret_cast<const EntityStateData *>(ptr);

            for (auto const &entity : entities) {
                auto &net = g_conductor.get_component<network>(entity);
                if (net.id == state->entity_id && !net.is_local) {
                    auto &trans = g_conductor.get_component<transform>(entity);
                    trans.position[0] = state->position_x;
                    trans.position[1] = state->position_y;

                    auto &rb = g_conductor.get_component<rigidbody>(entity);
                    rb.velocity[0] = state->velocity_x;
                    rb.velocity[1] = state->velocity_y;
                    break;
                }
            }

            ptr += sizeof(EntityStateData);
        }
        break;
    }
    default:
        break;
    }
}

// Network ID Management
void network_system::request_network_id() {
    if (NetworkManager::Get().IsHost()) {
        std::cerr << "Host cannot request network ID from itself" << std::endl;
        return;
    }

    ReserveNetworkIDRequestPacket packet;
    packet.header.type = PacketType::ReserveNetworkIDRequest;
    packet.header.sequence_number = 0;

    NetworkManager::Get().SendPacketToServer(&packet, sizeof(packet));
    std::cout << "Requested network ID from host" << std::endl;
}

entity network_system::create_networked_entity(uint32_t netId, bool is_local) {
    entity ent = g_conductor.create_entity();
    network net_comp;
    net_comp.id = netId;
    net_comp.is_local = is_local;
    g_conductor.add_component<network>(ent, net_comp);

    // Register in NetworkManager
    NetworkManager::Get().RegisterNetworkEntity(netId, ent);

    return ent;
}

void network_system::send_entity_init(entity ent) {
    // Serialize all components of the entity (except network component)
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(EntityInitPacketHeader));

    auto &net = g_conductor.get_component<network>(ent);

    EntityInitPacketHeader *header =
        reinterpret_cast<EntityInitPacketHeader *>(buffer.data());
    header->header.type = PacketType::EntityInitPacket;
    header->header.sequence_number = 0;
    header->network_id = net.id;
    header->component_count = 0;

    auto &serializer = ComponentSerializer::Get();

    // Track component count separately to avoid pointer invalidation
    uint32_t component_count = 0;

    // Helper lambda to serialize and append a component
    auto serialize_component = [&](auto &component, ComponentID id) {
        SerializedComponent sc;
        if (serializer.HasCustomSerializer<decltype(component)>()) {
            sc = serializer.SerializeCustom(component);
        } else {
            sc = serializer.Serialize(component, id);
        }

        uint8_t comp_id = static_cast<uint8_t>(sc.id);
        uint16_t comp_size = static_cast<uint16_t>(sc.data.size());

        buffer.push_back(comp_id);
        buffer.push_back(comp_size & 0xFF);
        buffer.push_back((comp_size >> 8) & 0xFF);
        buffer.insert(buffer.end(), sc.data.begin(), sc.data.end());

        component_count++;
    };

    // Serialize components if they exist
    // Use has_component to safely check before getting

    // Transform
    if (g_conductor.has_component<transform>(ent)) {
        auto &trans = g_conductor.get_component<transform>(ent);
        serialize_component(trans, ComponentID::Transform);
    }

    // Rigidbody
    if (g_conductor.has_component<rigidbody>(ent)) {
        auto &rb = g_conductor.get_component<rigidbody>(ent);
        serialize_component(rb, ComponentID::Rigidbody);
    }

    // Sprite
    if (g_conductor.has_component<sprite>(ent)) {
        auto &spr = g_conductor.get_component<sprite>(ent);
        serialize_component(spr, ComponentID::Sprite);
    }

    // Gravity
    if (g_conductor.has_component<gravity>(ent)) {
        auto &grav = g_conductor.get_component<gravity>(ent);
        serialize_component(grav, ComponentID::Gravity);
    }

    // Jump
    if (g_conductor.has_component<jump>(ent)) {
        auto &jmp = g_conductor.get_component<jump>(ent);
        serialize_component(jmp, ComponentID::Jump);
    }

    // Inventory
    if (g_conductor.has_component<inventory>(ent)) {
        auto &inv = g_conductor.get_component<inventory>(ent);
        serialize_component(inv, ComponentID::Inventory);
    }

    // Item
    if (g_conductor.has_component<item>(ent)) {
        auto &itm = g_conductor.get_component<item>(ent);
        serialize_component(itm, ComponentID::Item);
    }

    // Player
    if (g_conductor.has_component<player>(ent)) {
        auto &plr = g_conductor.get_component<player>(ent);
        serialize_component(plr, ComponentID::Player);
    }

    // Entity State
    if (g_conductor.has_component<entity_state>(ent)) {
        auto &state = g_conductor.get_component<entity_state>(ent);
        serialize_component(state, ComponentID::EntityState);
    }

    // Update header with final component count (use fresh pointer after all buffer modifications)
    EntityInitPacketHeader *final_header =
        reinterpret_cast<EntityInitPacketHeader *>(buffer.data());
    final_header->component_count = component_count;

    // Send to server (or broadcast if host)
    if (NetworkManager::Get().IsHost()) {
        NetworkManager::Get().BroadcastPacket(buffer.data(), buffer.size());
    } else {
        NetworkManager::Get().SendPacketToServer(buffer.data(), buffer.size());
    }

    std::cout << "Sent entity init for network ID " << net.id << " with "
              << component_count << " components" << std::endl;
}

void network_system::send_all_entities_to_client(HSteamNetConnection conn) {
    std::cout << "Sending all existing entities to new client..." << std::endl;

    for (auto const &ent : entities) {
        auto &net = g_conductor.get_component<network>(ent);
        if (!net.is_local)
            continue; // Only send local entities (entities this host owns)

        // Serialize all components of the entity
        std::vector<uint8_t> buffer;
        buffer.resize(sizeof(EntityInitPacketHeader));

        EntityInitPacketHeader *header =
            reinterpret_cast<EntityInitPacketHeader *>(buffer.data());
        header->header.type = PacketType::EntityInitPacket;
        header->header.sequence_number = 0;
        header->network_id = net.id;
        header->component_count = 0;

        auto &serializer = ComponentSerializer::Get();

        // Track component count separately to avoid pointer invalidation
        uint32_t component_count = 0;

        // Helper lambda to serialize and append a component
        auto serialize_component = [&](auto &component, ComponentID id) {
            SerializedComponent sc;
            if (serializer.HasCustomSerializer<decltype(component)>()) {
                sc = serializer.SerializeCustom(component);
            } else {
                sc = serializer.Serialize(component, id);
            }

            uint8_t comp_id = static_cast<uint8_t>(sc.id);
            uint16_t comp_size = static_cast<uint16_t>(sc.data.size());

            buffer.push_back(comp_id);
            buffer.push_back(comp_size & 0xFF);
            buffer.push_back((comp_size >> 8) & 0xFF);
            buffer.insert(buffer.end(), sc.data.begin(), sc.data.end());

            component_count++;
        };

        // Serialize all components (same as send_entity_init)
        if (g_conductor.has_component<transform>(ent)) {
            auto &trans = g_conductor.get_component<transform>(ent);
            serialize_component(trans, ComponentID::Transform);
        }
        if (g_conductor.has_component<rigidbody>(ent)) {
            auto &rb = g_conductor.get_component<rigidbody>(ent);
            serialize_component(rb, ComponentID::Rigidbody);
        }
        if (g_conductor.has_component<sprite>(ent)) {
            auto &spr = g_conductor.get_component<sprite>(ent);
            serialize_component(spr, ComponentID::Sprite);
        }
        if (g_conductor.has_component<gravity>(ent)) {
            auto &grav = g_conductor.get_component<gravity>(ent);
            serialize_component(grav, ComponentID::Gravity);
        }
        if (g_conductor.has_component<jump>(ent)) {
            auto &jmp = g_conductor.get_component<jump>(ent);
            serialize_component(jmp, ComponentID::Jump);
        }
        if (g_conductor.has_component<inventory>(ent)) {
            auto &inv = g_conductor.get_component<inventory>(ent);
            serialize_component(inv, ComponentID::Inventory);
        }
        if (g_conductor.has_component<item>(ent)) {
            auto &itm = g_conductor.get_component<item>(ent);
            serialize_component(itm, ComponentID::Item);
        }
        if (g_conductor.has_component<player>(ent)) {
            auto &plr = g_conductor.get_component<player>(ent);
            serialize_component(plr, ComponentID::Player);
        }
        if (g_conductor.has_component<entity_state>(ent)) {
            auto &state = g_conductor.get_component<entity_state>(ent);
            serialize_component(state, ComponentID::EntityState);
        }

        // Update header with final component count (use fresh pointer after all buffer modifications)
        EntityInitPacketHeader *final_header =
            reinterpret_cast<EntityInitPacketHeader *>(buffer.data());
        final_header->component_count = component_count;

        // Send to specific client
        NetworkManager::Get().SendToConnection(conn, buffer.data(), buffer.size());

        std::cout << "  - Sent entity " << net.id << " with "
                  << component_count << " components" << std::endl;
    }

    std::cout << "Finished sending all entities to new client" << std::endl;
}

void network_system::transfer_ownership(entity ent, uint32_t newOwnerPlayerId) {
    auto &net = g_conductor.get_component<network>(ent);

    OwnershipTransferPacketData packet;
    packet.header.type = PacketType::OwnershipTransferPacket;
    packet.header.sequence_number = 0;
    packet.network_id = net.id;
    packet.new_owner_player_id = newOwnerPlayerId;

    if (NetworkManager::Get().IsHost()) {
        NetworkManager::Get().BroadcastPacket(&packet, sizeof(packet));
    } else {
        NetworkManager::Get().SendPacketToServer(&packet, sizeof(packet));
    }

    std::cout << "Requested ownership transfer for network ID " << net.id
              << " to player " << newOwnerPlayerId << std::endl;
}

// Packet Handlers
void network_system::handle_reserve_id_request(HSteamNetConnection conn,
                                               const void *data, size_t size) {
    if (!NetworkManager::Get().IsHost())
        return;

    if (size < sizeof(ReserveNetworkIDRequestPacket))
        return;

    // Allocate a new network ID
    uint32_t newId = NetworkManager::Get().AllocateNetworkId();

    std::cout << "Host allocated network ID: " << newId << std::endl;

    // Broadcast to all clients that this ID is reserved
    NetworkIDReservedPacket reservedPacket;
    reservedPacket.header.type = PacketType::NetworkIDReserved;
    reservedPacket.header.sequence_number = 0;
    reservedPacket.reserved_id = newId;
    NetworkManager::Get().BroadcastPacket(&reservedPacket,
                                          sizeof(reservedPacket));

    // Send granted packet to requesting client
    NetworkIDGrantedPacket grantedPacket;
    grantedPacket.header.type = PacketType::NetworkIDGranted;
    grantedPacket.header.sequence_number = 0;
    grantedPacket.granted_id = newId;
    NetworkManager::Get().SendToConnection(conn, &grantedPacket,
                                           sizeof(grantedPacket));

    std::cout << "Granted network ID " << newId << " to client" << std::endl;

    // Send all existing entities to the new client
    send_all_entities_to_client(conn);
}

void network_system::handle_network_id_reserved(const void *data, size_t size) {
    if (size < sizeof(NetworkIDReservedPacket))
        return;

    const NetworkIDReservedPacket *packet =
        static_cast<const NetworkIDReservedPacket *>(data);

    m_reservedIds.push_back(packet->reserved_id);
    std::cout << "Network ID " << packet->reserved_id << " is now reserved"
              << std::endl;
}

void network_system::handle_network_id_granted(const void *data, size_t size) {
    if (size < sizeof(NetworkIDGrantedPacket))
        return;

    const NetworkIDGrantedPacket *packet =
        static_cast<const NetworkIDGrantedPacket *>(data);

    m_pendingGrantedId = packet->granted_id;
    std::cout << "Received granted network ID: " << packet->granted_id
              << std::endl;
}

void network_system::handle_entity_init(const void *data, size_t size) {
    if (size < sizeof(EntityInitPacketHeader))
        return;

    const EntityInitPacketHeader *header =
        static_cast<const EntityInitPacketHeader *>(data);

    std::cout << "Received entity init for network ID " << header->network_id
              << " with " << header->component_count << " components"
              << std::endl;

    // Check if entity already exists (prevent duplicates)
    entity existing_ent = NetworkManager::Get().GetEntityByNetworkId(header->network_id);
    if (existing_ent != 0) {
        std::cout << "Entity with network ID " << header->network_id
                  << " already exists, skipping creation" << std::endl;
        return;
    }

    // Create the entity with network component
    // The entity is remote (not local) since we're receiving it
    entity ent = create_networked_entity(header->network_id, false);

    // Deserialize components
    const uint8_t *ptr =
        static_cast<const uint8_t *>(data) + sizeof(EntityInitPacketHeader);
    auto &serializer = ComponentSerializer::Get();

    for (uint32_t i = 0; i < header->component_count; ++i) {
        if (ptr + 3 > static_cast<const uint8_t *>(data) + size)
            break;

        uint8_t comp_id_raw = *ptr++;
        uint16_t comp_size = *ptr++;
        comp_size |= (static_cast<uint16_t>(*ptr++) << 8);

        if (ptr + comp_size > static_cast<const uint8_t *>(data) + size)
            break;

        ComponentID comp_id = static_cast<ComponentID>(comp_id_raw);

        // Deserialize based on component ID
        switch (comp_id) {
        case ComponentID::Transform: {
            transform trans = serializer.Deserialize<transform>(ptr, comp_size);
            g_conductor.add_component<transform>(ent, trans);
            break;
        }
        case ComponentID::Rigidbody: {
            rigidbody rb =
                serializer.DeserializeCustom<rigidbody>(ptr, comp_size);
            g_conductor.add_component<rigidbody>(ent, rb);
            break;
        }
        case ComponentID::Sprite: {
            sprite spr = serializer.DeserializeCustom<sprite>(ptr, comp_size);
            // Load texture from texture_name
            if (!spr.texture_name.empty()) {
                if (!spr.texture.loadFromFile(spr.texture_name)) {
                    std::cerr << "Failed to load texture: " << spr.texture_name
                              << std::endl;
                } else {
                    spr.sprite_obj = sf::Sprite(spr.texture);
                }
            }
            g_conductor.add_component<sprite>(ent, spr);
            break;
        }
        case ComponentID::Gravity: {
            gravity grav = serializer.Deserialize<gravity>(ptr, comp_size);
            g_conductor.add_component<gravity>(ent, grav);
            break;
        }
        case ComponentID::Jump: {
            jump jmp = serializer.Deserialize<jump>(ptr, comp_size);
            g_conductor.add_component<jump>(ent, jmp);
            break;
        }
        case ComponentID::Inventory: {
            inventory inv =
                serializer.DeserializeCustom<inventory>(ptr, comp_size);
            g_conductor.add_component<inventory>(ent, inv);
            break;
        }
        case ComponentID::Item: {
            item itm = serializer.DeserializeCustom<item>(ptr, comp_size);
            g_conductor.add_component<item>(ent, itm);
            break;
        }
        case ComponentID::Player: {
            player plr = serializer.Deserialize<player>(ptr, comp_size);
            g_conductor.add_component<player>(ent, plr);
            break;
        }
        case ComponentID::EntityState: {
            entity_state state =
                serializer.Deserialize<entity_state>(ptr, comp_size);
            g_conductor.add_component<entity_state>(ent, state);
            break;
        }
        default:
            break;
        }

        ptr += comp_size;
    }

    // If we're the host, forward this to other clients
    if (NetworkManager::Get().IsHost()) {
        NetworkManager::Get().BroadcastPacket(data, size);
    }

    std::cout << "Created networked entity with ID " << header->network_id
              << std::endl;
}

void network_system::handle_ownership_transfer(const void *data, size_t size) {
    if (size < sizeof(OwnershipTransferPacketData))
        return;

    const OwnershipTransferPacketData *packet =
        static_cast<const OwnershipTransferPacketData *>(data);

    entity ent = NetworkManager::Get().GetEntityByNetworkId(packet->network_id);
    if (ent == 0)
        return;

    auto &net = g_conductor.get_component<network>(ent);

    // Check if we're the new owner
    if (packet->new_owner_player_id ==
        NetworkManager::Get().GetLocalPlayerId()) {
        net.is_local = true;
        std::cout << "Received ownership of entity " << packet->network_id
                  << std::endl;
    } else {
        net.is_local = false;
        std::cout << "Entity " << packet->network_id
                  << " transferred to player " << packet->new_owner_player_id
                  << std::endl;
    }

    // If we're the host, broadcast this to all clients
    if (NetworkManager::Get().IsHost()) {
        NetworkManager::Get().BroadcastPacket(data, size);
    }
}

void network_system::handle_component_batch_update(const void *data,
                                                   size_t size) {
    if (size < sizeof(ComponentBatchUpdatePacket))
        return;

    const ComponentBatchUpdatePacket *packet =
        static_cast<const ComponentBatchUpdatePacket *>(data);

    entity ent = NetworkManager::Get().GetEntityByNetworkId(packet->network_id);
    if (ent == 0)
        return;

    auto &net = g_conductor.get_component<network>(ent);
    if (net.is_local)
        return; // Don't apply updates to local entities

    // Deserialize components
    const uint8_t *ptr =
        static_cast<const uint8_t *>(data) + sizeof(ComponentBatchUpdatePacket);
    auto &serializer = ComponentSerializer::Get();

    for (uint32_t i = 0; i < packet->component_count; ++i) {
        if (ptr + 3 > static_cast<const uint8_t *>(data) + size)
            break;

        uint8_t comp_id_raw = *ptr++;
        uint16_t comp_size = *ptr++;
        comp_size |= (static_cast<uint16_t>(*ptr++) << 8);

        if (ptr + comp_size > static_cast<const uint8_t *>(data) + size)
            break;

        ComponentID comp_id = static_cast<ComponentID>(comp_id_raw);

        // Deserialize and apply based on component ID
        switch (comp_id) {
        case ComponentID::Transform: {
            transform trans = serializer.Deserialize<transform>(ptr, comp_size);
            if (g_conductor.has_component<transform>(ent)) {
                g_conductor.get_component<transform>(ent) = trans;
            } else {
                g_conductor.add_component<transform>(ent, trans);
            }
            break;
        }
        case ComponentID::Rigidbody: {
            rigidbody rb =
                serializer.DeserializeCustom<rigidbody>(ptr, comp_size);
            if (g_conductor.has_component<rigidbody>(ent)) {
                g_conductor.get_component<rigidbody>(ent) = rb;
            } else {
                g_conductor.add_component<rigidbody>(ent, rb);
            }
            break;
        }
        case ComponentID::Sprite: {
            sprite spr = serializer.DeserializeCustom<sprite>(ptr, comp_size);
            // Load texture from texture_name
            if (!spr.texture_name.empty()) {
                if (!spr.texture.loadFromFile(spr.texture_name)) {
                    std::cerr << "Failed to load texture: " << spr.texture_name
                              << std::endl;
                } else {
                    spr.sprite_obj = sf::Sprite(spr.texture);
                }
            }
            if (g_conductor.has_component<sprite>(ent)) {
                g_conductor.get_component<sprite>(ent) = spr;
            } else {
                g_conductor.add_component<sprite>(ent, spr);
            }
            break;
        }
        case ComponentID::Gravity: {
            gravity grav = serializer.Deserialize<gravity>(ptr, comp_size);
            if (g_conductor.has_component<gravity>(ent)) {
                g_conductor.get_component<gravity>(ent) = grav;
            } else {
                g_conductor.add_component<gravity>(ent, grav);
            }
            break;
        }
        case ComponentID::Jump: {
            jump jmp = serializer.Deserialize<jump>(ptr, comp_size);
            if (g_conductor.has_component<jump>(ent)) {
                g_conductor.get_component<jump>(ent) = jmp;
            } else {
                g_conductor.add_component<jump>(ent, jmp);
            }
            break;
        }
        case ComponentID::Inventory: {
            inventory inv =
                serializer.DeserializeCustom<inventory>(ptr, comp_size);
            if (g_conductor.has_component<inventory>(ent)) {
                g_conductor.get_component<inventory>(ent) = inv;
            } else {
                g_conductor.add_component<inventory>(ent, inv);
            }
            break;
        }
        case ComponentID::Item: {
            item itm = serializer.DeserializeCustom<item>(ptr, comp_size);
            if (g_conductor.has_component<item>(ent)) {
                g_conductor.get_component<item>(ent) = itm;
            } else {
                g_conductor.add_component<item>(ent, itm);
            }
            break;
        }
        case ComponentID::Player: {
            player plr = serializer.Deserialize<player>(ptr, comp_size);
            if (g_conductor.has_component<player>(ent)) {
                g_conductor.get_component<player>(ent) = plr;
            } else {
                g_conductor.add_component<player>(ent, plr);
            }
            break;
        }
        case ComponentID::EntityState: {
            entity_state state =
                serializer.Deserialize<entity_state>(ptr, comp_size);
            if (g_conductor.has_component<entity_state>(ent)) {
                g_conductor.get_component<entity_state>(ent) = state;
            } else {
                g_conductor.add_component<entity_state>(ent, state);
            }
            break;
        }
        default:
            break;
        }

        ptr += comp_size;
    }

    // If we're the host, forward this to other clients
    if (NetworkManager::Get().IsHost()) {
        NetworkManager::Get().BroadcastPacket(data, size,
                                              k_nSteamNetworkingSend_Unreliable);
    }
}

// Network sync methods
bool network_system::has_component_changed(entity ent, ComponentID comp_id,
                                           const std::vector<uint8_t> &current_data) {
    auto entity_it = m_lastSentComponentData.find(ent);
    if (entity_it == m_lastSentComponentData.end()) {
        return true; // No previous data, consider it changed
    }

    auto comp_it = entity_it->second.find(comp_id);
    if (comp_it == entity_it->second.end()) {
        return true; // No previous data for this component
    }

    // Compare the data
    return comp_it->second != current_data;
}

void network_system::broadcast_component_updates() {
    auto &serializer = ComponentSerializer::Get();

    for (auto const &ent : entities) {
        auto &net = g_conductor.get_component<network>(ent);
        if (!net.is_local)
            continue; // Only send local entities

        // Build buffer for this entity's component updates
        std::vector<uint8_t> buffer;
        buffer.resize(sizeof(ComponentBatchUpdatePacket));

        ComponentBatchUpdatePacket *header =
            reinterpret_cast<ComponentBatchUpdatePacket *>(buffer.data());
        header->header.type = PacketType::ComponentBatchUpdate;
        header->header.sequence_number = 0;
        header->network_id = net.id;
        header->component_count = 0;

        // Track component count separately to avoid pointer invalidation
        uint32_t component_count = 0;

        // Iterate through networked components
        for (ComponentID comp_id : net.networked_components) {
            std::vector<uint8_t> serialized_data;
            bool has_component = false;

            // Serialize based on component type
            switch (comp_id) {
            case ComponentID::Transform:
                if (g_conductor.has_component<transform>(ent)) {
                    auto &comp = g_conductor.get_component<transform>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Rigidbody:
                if (g_conductor.has_component<rigidbody>(ent)) {
                    auto &comp = g_conductor.get_component<rigidbody>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Sprite:
                if (g_conductor.has_component<sprite>(ent)) {
                    auto &comp = g_conductor.get_component<sprite>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Gravity:
                if (g_conductor.has_component<gravity>(ent)) {
                    auto &comp = g_conductor.get_component<gravity>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Jump:
                if (g_conductor.has_component<jump>(ent)) {
                    auto &comp = g_conductor.get_component<jump>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Inventory:
                if (g_conductor.has_component<inventory>(ent)) {
                    auto &comp = g_conductor.get_component<inventory>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Item:
                if (g_conductor.has_component<item>(ent)) {
                    auto &comp = g_conductor.get_component<item>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Player:
                if (g_conductor.has_component<player>(ent)) {
                    auto &comp = g_conductor.get_component<player>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::EntityState:
                if (g_conductor.has_component<entity_state>(ent)) {
                    auto &comp = g_conductor.get_component<entity_state>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            default:
                break;
            }

            // Check if component changed
            if (has_component && has_component_changed(ent, comp_id, serialized_data)) {
                // Add to buffer: [comp_id][size][data]
                uint8_t comp_id_raw = static_cast<uint8_t>(comp_id);
                uint16_t comp_size = static_cast<uint16_t>(serialized_data.size());

                buffer.push_back(comp_id_raw);
                buffer.push_back(comp_size & 0xFF);
                buffer.push_back((comp_size >> 8) & 0xFF);
                buffer.insert(buffer.end(), serialized_data.begin(), serialized_data.end());

                component_count++;

                // Update last sent data
                m_lastSentComponentData[ent][comp_id] = serialized_data;
            }
        }

        // Send packet if there are changes
        if (component_count > 0) {
            // Update header with final component count (use fresh pointer after all buffer modifications)
            ComponentBatchUpdatePacket *final_header =
                reinterpret_cast<ComponentBatchUpdatePacket *>(buffer.data());
            final_header->component_count = component_count;

            NetworkManager::Get().BroadcastPacket(buffer.data(), buffer.size(),
                                                  k_nSteamNetworkingSend_Unreliable);
        }
    }
}

void network_system::send_component_updates() {
    auto &serializer = ComponentSerializer::Get();

    for (auto const &ent : entities) {
        auto &net = g_conductor.get_component<network>(ent);
        if (!net.is_local)
            continue; // Only send local entities

        // Build buffer for this entity's component updates
        std::vector<uint8_t> buffer;
        buffer.resize(sizeof(ComponentBatchUpdatePacket));

        ComponentBatchUpdatePacket *header =
            reinterpret_cast<ComponentBatchUpdatePacket *>(buffer.data());
        header->header.type = PacketType::ComponentBatchUpdate;
        header->header.sequence_number = 0;
        header->network_id = net.id;
        header->component_count = 0;

        // Track component count separately to avoid pointer invalidation
        uint32_t component_count = 0;

        // Iterate through networked components
        for (ComponentID comp_id : net.networked_components) {
            std::vector<uint8_t> serialized_data;
            bool has_component = false;

            // Serialize based on component type
            switch (comp_id) {
            case ComponentID::Transform:
                if (g_conductor.has_component<transform>(ent)) {
                    auto &comp = g_conductor.get_component<transform>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Rigidbody:
                if (g_conductor.has_component<rigidbody>(ent)) {
                    auto &comp = g_conductor.get_component<rigidbody>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Sprite:
                if (g_conductor.has_component<sprite>(ent)) {
                    auto &comp = g_conductor.get_component<sprite>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Gravity:
                if (g_conductor.has_component<gravity>(ent)) {
                    auto &comp = g_conductor.get_component<gravity>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Jump:
                if (g_conductor.has_component<jump>(ent)) {
                    auto &comp = g_conductor.get_component<jump>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Inventory:
                if (g_conductor.has_component<inventory>(ent)) {
                    auto &comp = g_conductor.get_component<inventory>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Item:
                if (g_conductor.has_component<item>(ent)) {
                    auto &comp = g_conductor.get_component<item>(ent);
                    SerializedComponent sc = serializer.SerializeCustom(comp);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::Player:
                if (g_conductor.has_component<player>(ent)) {
                    auto &comp = g_conductor.get_component<player>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            case ComponentID::EntityState:
                if (g_conductor.has_component<entity_state>(ent)) {
                    auto &comp = g_conductor.get_component<entity_state>(ent);
                    SerializedComponent sc = serializer.Serialize(comp, comp_id);
                    serialized_data = sc.data;
                    has_component = true;
                }
                break;
            default:
                break;
            }

            // Check if component changed
            if (has_component && has_component_changed(ent, comp_id, serialized_data)) {
                // Add to buffer: [comp_id][size][data]
                uint8_t comp_id_raw = static_cast<uint8_t>(comp_id);
                uint16_t comp_size = static_cast<uint16_t>(serialized_data.size());

                buffer.push_back(comp_id_raw);
                buffer.push_back(comp_size & 0xFF);
                buffer.push_back((comp_size >> 8) & 0xFF);
                buffer.insert(buffer.end(), serialized_data.begin(), serialized_data.end());

                component_count++;

                // Update last sent data
                m_lastSentComponentData[ent][comp_id] = serialized_data;
            }
        }

        // Send packet to host if there are changes
        if (component_count > 0) {
            // Update header with final component count (use fresh pointer after all buffer modifications)
            ComponentBatchUpdatePacket *final_header =
                reinterpret_cast<ComponentBatchUpdatePacket *>(buffer.data());
            final_header->component_count = component_count;

            NetworkManager::Get().SendPacketToServer(buffer.data(), buffer.size(),
                                                     k_nSteamNetworkingSend_Unreliable);
        }
    }
}

// Old methods (kept for compatibility)
void network_system::broadcast_state() {
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(GameStateUpdatePacket) +
                  entities.size() * sizeof(EntityStateData));

    GameStateUpdatePacket *packet =
        reinterpret_cast<GameStateUpdatePacket *>(buffer.data());
    packet->header.type = PacketType::GameStateUpdate;
    packet->header.sequence_number = 0;
    packet->entity_count = 0;

    uint8_t *ptr = buffer.data() + sizeof(GameStateUpdatePacket);

    for (auto const &entity : entities) {
        auto &net = g_conductor.get_component<network>(entity);
        if (net.is_local) { // Only send authoritative entities
            packet->entity_count++;
            EntityStateData *data = reinterpret_cast<EntityStateData *>(ptr);
            data->entity_id = net.id;

            auto &trans = g_conductor.get_component<transform>(entity);
            data->position_x = trans.position[0];
            data->position_y = trans.position[1];

            auto &rb = g_conductor.get_component<rigidbody>(entity);
            data->velocity_x = rb.velocity[0];
            data->velocity_y = rb.velocity[1];

            ptr += sizeof(EntityStateData);
        }
    }

    if (packet->entity_count > 0) {
        NetworkManager::Get().BroadcastPacket(
            buffer.data(),
            sizeof(GameStateUpdatePacket) +
                packet->entity_count * sizeof(EntityStateData),
            k_nSteamNetworkingSend_Unreliable);
    }
}

void network_system::send_input() {
    PlayerInputPacket packet;
    packet.header.type = PacketType::PlayerInput;
    packet.header.sequence_number = 0;

    packet.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    packet.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
    packet.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    packet.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
    packet.jump = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    NetworkManager::Get().SendPacketToServer(&packet, sizeof(packet),
                                             k_nSteamNetworkingSend_Unreliable);
}

void network_system::update_remote_entities(float dt) {
    // Smooth movement handled by interpolate_network_components
}
