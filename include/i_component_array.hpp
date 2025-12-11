#pragma once

#include "entity.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <unordered_map>

class i_component_array {
public:
  virtual ~i_component_array() = default;
  virtual void entity_destroyed(entity entity) = 0;
};

template <typename T> class component_array : public i_component_array {
public:
  void insert_data(entity ent, T component);
  void remove_data(entity ent);
  T &get_data(entity ent);
  bool has_data(entity ent);
  void entity_destroyed(entity ent) override;

private:
  /*
   * The packed array of components (of generic type T),
   * set to a specified maximum amount, matching the maximum number
   * of entities allowed to exist simultaneously, so that each entity
   * has a unique spot.
   */
  std::array<T, MAX_ENTITIES> m_component_array;
  // Map from an entity ID to an array index.
  std::unordered_map<entity, size_t> entity_to_index_map;
  // Map from an array index to an entity ID.
  std::unordered_map<size_t, entity> index_to_entity_map;
  // Total size of valid entries in the array.
  size_t size;
};

// Template function definitions
template <typename T>
void component_array<T>::insert_data(entity ent, T component) {
  assert(entity_to_index_map.find(ent) == entity_to_index_map.end() &&
         "Component added to same entity more than once.");

  // Put new entry at end and update the maps
  size_t new_index = size;
  entity_to_index_map[ent] = new_index;
  index_to_entity_map[new_index] = ent;
  m_component_array[new_index] = component;
  ++size;
}

template <typename T> void component_array<T>::remove_data(entity ent) {
  assert(entity_to_index_map.find(ent) != entity_to_index_map.end() &&
         "Removing non-existent component.");

  // Copy element at end into deleted element's place to maintain density
  size_t index_of_removed_entity = entity_to_index_map[ent];
  size_t index_of_last_element = size - 1;
  m_component_array[index_of_removed_entity] =
      m_component_array[index_of_last_element];

  // Update map to point to moved spot
  entity entity_of_last_element = index_to_entity_map[index_of_last_element];
  entity_to_index_map[entity_of_last_element] = index_of_removed_entity;
  index_to_entity_map[index_of_removed_entity] = entity_of_last_element;

  entity_to_index_map.erase(ent);
  index_to_entity_map.erase(index_of_last_element);

  --size;
}

template <typename T> T &component_array<T>::get_data(entity ent) {
  assert(entity_to_index_map.find(ent) != entity_to_index_map.end() &&
         "Retrieving non-existent component.");

  // Return a reference to the entity's component
  return m_component_array[entity_to_index_map[ent]];
}

template <typename T> bool component_array<T>::has_data(entity ent) {
  return entity_to_index_map.find(ent) != entity_to_index_map.end();
}

template <typename T> void component_array<T>::entity_destroyed(entity ent) {
  if (entity_to_index_map.find(ent) != entity_to_index_map.end()) {
    // Remove the entity's component if it existed
    remove_data(ent);
  }
}