#include <memory>
#include <cassert>
#include "system_manager.hpp"

void system_manager::entity_destroyed(entity entity)
{
    // Erase a destroyed entity from all system lists
    // mEntities is a set so no check needed
    for (auto const& pair : systems)
    {
        auto const& system = pair.second;

        system->entities.erase(entity);
    }
}

void system_manager::entity_signature_changed(entity entity, signature entity_signature)
{
    // Notify each system that an entity's signature changed
    for (auto const& pair : systems)
    {
        auto const& type = pair.first;
        auto const& system = pair.second;
        auto const& systemSignature = signatures[type];

        // Entity signature matches system signature - insert into set
        if ((entity_signature & systemSignature) == systemSignature) system->entities.insert(entity);
        // Entity signature does not match system signature - erase from set
        else system->entities.erase(entity);
    }
}
