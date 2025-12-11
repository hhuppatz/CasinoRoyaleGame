#include "conductor.hpp"
#include "components/network.hpp"
#include "entity.hpp"
#include "network_manager.hpp"
#include <cstdint>
#include <memory>

void conductor::init() {
    // Create pointers to each manager
    m_component_manager = std::make_unique<component_manager>();
    m_entity_manager = std::make_unique<entity_manager>();
    m_system_manager = std::make_unique<system_manager>();
}

// Entity methods
entity conductor::create_entity() { return m_entity_manager->create_entity(); }

void conductor::destroy_entity(entity entity) {
    m_entity_manager->destroy_entity(entity);
    m_component_manager->entity_destroyed(entity);
    m_system_manager->entity_destroyed(entity);
}

// Special case for creating a networked entity
// This is used to create a networked entity with a given network ID and local flag
// The network ID is used to identify the entity on the network
// The local flag is used to indicate if the entity is local to the current machine
// This is used to determine if the entity should be synced to other machines
// This is used to determine if the entity should be controlled by the local player
// This is used to determine if the entity should be controlled by the network
// This is used to determine if the entity should be controlled by the server
entity conductor::create_networked_entity(uint32_t network_id, bool is_local) {
    entity ent = create_entity();

    // Add network component
    network net_comp;
    net_comp.id = network_id;
    net_comp.is_local = is_local;

    add_component<network>(ent, net_comp);

    // Register with NetworkManager
    NetworkManager::Get().RegisterNetworkEntity(network_id, ent);

    return ent;
}
