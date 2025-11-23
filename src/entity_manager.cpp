#include "entity_manager.hpp"

entity_manager::entity_manager() {
    for (entity i = 0; i < MAX_ENTITIES; i++) {
        available_entities.push(i); // Add all entities to the available entities queue
    }
}

entity entity_manager::create_entity() {
    entity id = available_entities.front(); // Get the first available entity
    available_entities.pop(); // Remove the entity from the available entities queue
    living_entity_count++; // Increment the living entity count
    return id;
}

void entity_manager::destroy_entity(entity entity) {
    signatures[entity].reset(); // Reset the signature
    available_entities.push(entity); // Add the entity to the available entities queue
    living_entity_count--; // Decrement the living entity count
}

signature entity_manager::get_signature(entity entity) {
    return signatures[entity]; // Return the signature for the entity
}

void entity_manager::set_signature(entity entity, signature signature) {
    signatures[entity] = signature; // Set the signature for the entity
}