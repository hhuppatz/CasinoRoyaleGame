#include <array>
#include <cstdint>
#include <queue>
#include "entity.hpp"
#include "component.hpp"

#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

class entity_manager {
    public:
        entity_manager();
        entity create_entity();
        void destroy_entity(entity entity);
        signature get_signature(entity entity);
        void set_signature(entity entity, signature signature);

    private:
        // Queue of unused entity IDs
        std::queue<entity> available_entities{};
    
        // Array of signatures where the index corresponds to the entity ID
        std::array<signature, MAX_ENTITIES> signatures{};
    
        // Total living entities - used to keep limits on how many exist
        std::uint32_t living_entity_count{};
};

#endif