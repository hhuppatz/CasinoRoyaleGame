#include "component_manager.hpp"
#include <cassert>
#include <memory>

void component_manager::entity_destroyed(entity entity)
{
    // Notify each component array that an entity has been destroyed
    // If it has a component for that entity, it will remove it
    for (auto const& pair : component_arrays)
    {
        auto const& component = pair.second;

        component->entity_destroyed(entity);
    }
}
