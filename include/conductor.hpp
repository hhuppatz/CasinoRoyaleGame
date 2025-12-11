#pragma once

#include "component_manager.hpp"
#include "entity.hpp"
#include "entity_manager.hpp"
#include "system_manager.hpp"
#include <memory>

class conductor {
public:
  void init();
  entity create_entity();
  void destroy_entity(entity entity);
  entity create_networked_entity(uint32_t network_id, bool is_local);
  template <typename T> void register_component();
  template <typename T> void add_component(entity entity, T component);
  template <typename T> void remove_component(entity entity);
  template <typename T> T &get_component(entity entity);
  template <typename T> bool has_component(entity entity);
  template <typename T> component_type get_component_type();
  template <typename T> std::shared_ptr<T> register_system();
  template <typename T> void set_system_signature(signature signature);

private:
  std::unique_ptr<component_manager> m_component_manager;
  std::unique_ptr<entity_manager> m_entity_manager;
  std::unique_ptr<system_manager> m_system_manager;
};

// Template function definitions
template <typename T> void conductor::register_component() {
  m_component_manager->register_component<T>();
}

template <typename T>
void conductor::add_component(entity entity, T component) {
  m_component_manager->add_component<T>(entity, component);

  auto signature = m_entity_manager->get_signature(entity);
  signature.set(m_component_manager->get_component_type<T>(), true);
  m_entity_manager->set_signature(entity, signature);

  m_system_manager->entity_signature_changed(entity, signature);
}

template <typename T> void conductor::remove_component(entity entity) {
  m_component_manager->remove_component<T>(entity);

  auto signature = m_entity_manager->get_signature(entity);
  signature.set(m_component_manager->get_component_type<T>(), false);
  m_entity_manager->set_signature(entity, signature);

  m_system_manager->entity_signature_changed(entity, signature);
}

template <typename T> T &conductor::get_component(entity entity) {
  return m_component_manager->get_component<T>(entity);
}

template <typename T> bool conductor::has_component(entity entity) {
  return m_component_manager->has_component<T>(entity);
}

template <typename T> component_type conductor::get_component_type() {
  return m_component_manager->get_component_type<T>();
}

template <typename T> std::shared_ptr<T> conductor::register_system() {
  return m_system_manager->register_system<T>();
}

template <typename T>
void conductor::set_system_signature(signature signature) {
  m_system_manager->set_signature<T>(signature);
}