# Networked Entity Creation - Usage Guide

This document describes how to use the newly implemented networked entity creation system.

## Overview

The system implements a two-phase networked entity creation flow:
1. **ID Reservation**: Client requests and receives a unique network ID from the host
2. **Entity Initialization**: Client creates the entity with components and broadcasts initialization data

## Architecture

### Components

- **`network`**: Marks an entity as networked with a unique ID and ownership flag (`is_local`)
- **`network_position`**: Network-synchronized position with interpolation support
- **`network_rigidbody`**: Network-synchronized velocity with interpolation support

### Systems

- **`network_system`**: Handles packet processing, ID reservation, and network updates
- **`network_sync_system`**: Syncs between gameplay components (transform/rigidbody) and network components

### Packet Flow

```
Client                          Host                         Other Clients
  |                              |                                 |
  |--ReserveNetworkIDRequest---->|                                 |
  |                              |---NetworkIDReserved------------>|
  |<---NetworkIDGranted----------|                                 |
  |                              |                                 |
  | (create entity locally)      |                                 |
  |                              |                                 |
  |--EntityInitPacket----------->|                                 |
  |                              |---EntityInitPacket------------->|
  |                              |                                 |
  | (every frame)                |                                 |
  |--NetworkPositionUpdate------>|                                 |
  |--NetworkRigidbodyUpdate----->|                                 |
  |                              |---NetworkPositionUpdate-------->|
  |                              |---NetworkRigidbodyUpdate------->|
```

## Usage Example

### Client-Side: Creating a Networked Entity

```cpp
// Step 1: Request a network ID
network_system1->request_network_id();

// Wait for the grant (check in game loop)
if (network_system1->has_pending_granted_id()) {
  uint32_t granted_id = network_system1->get_pending_granted_id();
  network_system1->clear_pending_granted_id();
  
  // Step 2: Create the entity with network component
  entity new_ent = g_conductor.create_networked_entity(granted_id, true);
  
  // Step 3: Add gameplay components
  g_conductor.add_component<transform>(
      new_ent, transform{{100.0f, 100.0f}, {100.0f, 100.0f}, {1.0f, 1.0f}});
  g_conductor.add_component<rigidbody>(
      new_ent, rigidbody{{0.0f, 0.0f}, 50.0f, 
                         sf::RectangleShape({32.0f, 32.0f}), 
                         true, {32.0f, 32.0f}});
  g_conductor.add_component<sprite>(new_ent, sprite_comp);
  
  // Step 4: Add network sync components
  g_conductor.add_component<network_position>(
      new_ent, network_position{100.0f, 100.0f, 100.0f, 100.0f, 
                                100.0f, 100.0f, 0.0f});
  g_conductor.add_component<network_rigidbody>(
      new_ent, network_rigidbody{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  
  // Step 5: Send initialization to host (and other clients)
  network_system1->send_entity_init(new_ent);
}
```

### Alternative: Using Conductor Helper

```cpp
// The conductor provides a helper method that creates entity + network component
if (network_system1->has_pending_granted_id()) {
  uint32_t granted_id = network_system1->get_pending_granted_id();
  network_system1->clear_pending_granted_id();
  
  // This is equivalent to network_system1->create_networked_entity()
  entity new_ent = g_conductor.create_networked_entity(granted_id, true);
  
  // ... add other components and send init ...
}
```

## How It Works

### ID Reservation Flow

1. **Client** calls `network_system->request_network_id()`
2. **Host** receives `ReserveNetworkIDRequest`
3. **Host** allocates new ID from counter (`m_nextNetworkId++`)
4. **Host** broadcasts `NetworkIDReserved` to all clients (marks ID as taken)
5. **Host** sends `NetworkIDGranted` to requesting client
6. **Client** stores granted ID in `m_pendingGrantedId`

### Entity Initialization Flow

1. **Client** creates entity locally with `create_networked_entity(id, true)`
   - Sets `is_local = true` (this client has authority)
2. **Client** adds gameplay components (transform, rigidbody, sprite, etc.)
3. **Client** calls `send_entity_init(entity)`
   - Serializes all components (except network component)
   - Sends `EntityInitPacket` to host
4. **Host** receives packet and creates matching entity with `is_local = false`
5. **Host** forwards `EntityInitPacket` to other clients
6. **Other Clients** create matching entities with `is_local = false`

### Network Synchronization

Every frame:

1. **network_sync_system** runs:
   - For **local entities** (`is_local = true`):
     - Copies `transform` → `network_position`
     - Copies `rigidbody` → `network_rigidbody`
   - For **remote entities** (`is_local = false`):
     - Copies `network_position` → `transform`
     - Copies `network_rigidbody` → `rigidbody`

2. **network_system** runs:
   - For **local entities**:
     - Broadcasts `NetworkPositionUpdate` and `NetworkRigidbodyUpdate`
   - For **remote entities**:
     - Receives updates and stores in network components
     - Interpolates between `last_*` and `target_*` values

### Ownership Transfer

Transfer entity authority to another player:

```cpp
// Transfer ownership of entity to player ID 2
network_system1->transfer_ownership(entity, 2);
```

This will:
1. Send `OwnershipTransferPacket` to host
2. Host broadcasts to all clients
3. Matching player sets `is_local = true`, others set `is_local = false`

## Component Serialization

The system uses a generic serialization framework that supports:

### Simple Components (POD types)
- `transform`, `gravity`, `jump` - serialized via memcpy

### Complex Components (custom serializers)
- **`sprite`**: Only texture name is serialized (texture loaded on receive)
- **`rigidbody`**: Hitbox not serialized (reconstructed from base_size)
- **`item`**: UI sprite not serialized (needs manual setup after creation)
- **`inventory`**: Entity references not serialized (only counts)

### Adding New Serializable Components

1. Add ComponentID to enum in `component_serialization.hpp`
2. Register serializer in `InitializeComponentSerializers()` (called in main)
3. Add case in `handle_entity_init()` to deserialize

## Important Notes

### Entity Authority

- **Local entities** (`is_local = true`): This machine has authority
  - Position/velocity updates are SENT to network
  - Gameplay systems update these entities normally
  
- **Remote entities** (`is_local = false`): Another machine has authority
  - Position/velocity updates are RECEIVED from network
  - Local physics/input systems should NOT modify these entities

### Interpolation

Remote entities use interpolation to smooth movement:
- Network updates set `target_x/y` and reset `t = 0`
- Each frame, `t` increases and position interpolates
- This prevents jittery movement from discrete network updates

### Texture Loading

When receiving `EntityInitPacket` with a sprite component:
- Only the texture filename is transmitted
- **You must manually load the texture** from the filename
- Example: Check `handle_entity_init()` - it sets `texture_name` but doesn't load

### Host Behavior

The host:
- Allocates network IDs sequentially from 1
- Forwards all `EntityInitPacket` messages to other clients
- Broadcasts network updates for all entities
- Acts as authoritative source for ID reservation

## Testing Checklist

- [ ] Host can start and allocate IDs
- [ ] Client can request and receive network ID
- [ ] Client can create entity and send initialization
- [ ] Host and other clients receive and create matching entities
- [ ] Position/velocity sync works correctly
- [ ] Interpolation smooths remote entity movement
- [ ] Ownership transfer works between clients

## Troubleshooting

**ID never granted**: Check network connection and packet callback setup

**Entity not created on remote**: Verify component serializers are initialized

**Jittery movement**: Check interpolation factor and network update rate

**Texture missing**: Load texture from `sprite.texture_name` after deserialization

