#include "conductor.hpp"
#include <memory>

void conductor::init()
{
    // Create pointers to each manager
    m_component_manager = std::make_unique<component_manager>();
    m_entity_manager = std::make_unique<entity_manager>();
    m_system_manager = std::make_unique<system_manager>();
}

// Entity methods
entity conductor::create_entity()
{
    return m_entity_manager->create_entity();
}

void conductor::destroy_entity(entity entity)
{
    m_entity_manager->destroy_entity(entity);
    m_component_manager->entity_destroyed(entity);
    m_system_manager->entity_destroyed(entity);
}

