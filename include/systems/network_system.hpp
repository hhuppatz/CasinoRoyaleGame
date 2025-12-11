#pragma once

#include "component_serialization.hpp"
#include "entity.hpp"
#include "network_manager.hpp"
#include "system_manager.hpp"
#include <cstdint>
#include <map>
#include <vector>

class network_system : public game_system {
public:
  void update(float dt);
  void handle_packet(HSteamNetConnection conn, const void *data, size_t size);

  // Network ID management
  void request_network_id();
  entity create_networked_entity(uint32_t netId, bool is_local);
  void send_entity_init(entity ent);
  void transfer_ownership(entity ent, uint32_t newOwnerPlayerId);

  // Get pending granted ID (for clients after receiving grant)
  uint32_t get_pending_granted_id() const { return m_pendingGrantedId; }
  bool has_pending_granted_id() const { return m_pendingGrantedId != 0; }
  void clear_pending_granted_id() { m_pendingGrantedId = 0; }

private:
  // Old methods
  void broadcast_state();
  void send_input();
  void update_remote_entities(float dt);

  // New packet handlers
  void handle_reserve_id_request(HSteamNetConnection conn, const void *data,
                                 size_t size);
  void handle_network_id_reserved(const void *data, size_t size);
  void handle_network_id_granted(const void *data, size_t size);
  void handle_entity_init(const void *data, size_t size);
  void handle_ownership_transfer(const void *data, size_t size);
  void handle_component_batch_update(const void *data, size_t size);

  // Network sync
  void broadcast_component_updates();
  void send_component_updates();
  void send_all_entities_to_client(HSteamNetConnection conn);
  bool has_component_changed(entity ent, ComponentID comp_id, 
                             const std::vector<uint8_t>& current_data);

  // State
  uint32_t m_pendingGrantedId = 0; // For clients waiting for ID grant
  std::vector<uint32_t> m_reservedIds; // IDs that are in use
  std::map<entity, std::map<ComponentID, std::vector<uint8_t>>> m_lastSentComponentData; // Change tracking
};
