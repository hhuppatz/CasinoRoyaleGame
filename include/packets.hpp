#pragma once
#include <array>
#include <cstdint>


enum class PacketType : uint8_t {
  JoinRequest,
  JoinAccept,
  PlayerInput,
  GameStateUpdate,
  ReserveNetworkIDRequest,
  NetworkIDReserved,
  NetworkIDGranted,
  EntityInitPacket,
  ComponentBatchUpdate,
  OwnershipTransferPacket
};

#pragma pack(push, 1)

struct PacketHeader {
  PacketType type;
  uint32_t sequence_number;
};

struct JoinRequestPacket {
  PacketHeader header;
  // Could add version check or password here
};

struct JoinAcceptPacket {
  PacketHeader header;
  uint32_t assigned_player_id;
  // Could add map seed or initial state here
};

struct PlayerInputPacket {
  PacketHeader header;
  bool up;
  bool down;
  bool left;
  bool right;
  bool jump;
  // Add other inputs as needed
};

struct EntityStateData {
  uint32_t entity_id;
  float position_x;
  float position_y;
  float velocity_x;
  float velocity_y;
  // Add other state data like animation frame, etc.
};

struct GameStateUpdatePacket {
  PacketHeader header;
  uint32_t entity_count;
  // Followed by entity_count * EntityStateData
  // We'll handle the variable size payload in serialization
};

// Network ID Reservation Packets
struct ReserveNetworkIDRequestPacket {
  PacketHeader header;
  // Client requests a network ID
};

struct NetworkIDReservedPacket {
  PacketHeader header;
  uint32_t reserved_id;
  // Host broadcasts that this ID is now reserved
};

struct NetworkIDGrantedPacket {
  PacketHeader header;
  uint32_t granted_id;
  // Host grants ownership of this ID to requesting client
};

// Entity Initialization Packet
struct EntityInitPacketHeader {
  PacketHeader header;
  uint32_t network_id;
  uint32_t component_count;
  // Followed by serialized components:
  // For each component: [component_id (uint8_t)][size (uint16_t)][data]
};

// Component Batch Update Packet
struct ComponentBatchUpdatePacket {
  PacketHeader header;
  uint32_t network_id;           // Entity to update
  uint32_t component_count;      // Number of components in this batch
  // Followed by serialized components:
  // For each component: [component_id (uint8_t)][size (uint16_t)][data]
};

// Ownership Transfer Packet
struct OwnershipTransferPacketData {
  PacketHeader header;
  uint32_t network_id;
  uint32_t new_owner_player_id;
};

#pragma pack(pop)
