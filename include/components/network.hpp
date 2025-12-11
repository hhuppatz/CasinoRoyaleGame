#pragma once

#include "component_serialization.hpp"
#include <cstdint>
#include <vector>

struct network {
    uint32_t id;
    bool is_local; // True if this entity is authoritative on this machine (e.g.
                   // local player on client, all entities on host)
    std::vector<ComponentID> networked_components; // List of components to sync over network
};
