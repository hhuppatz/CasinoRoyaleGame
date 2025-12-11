# Networked Player Implementation

This document describes the changes made to convert the local player entity into a networked entity that synchronizes across clients.

## Overview

The player entity is now fully networked, allowing hosts and clients to see each other running around in the game. The implementation follows the two-phase networked entity creation flow described in `NETWORKED_ENTITY_CREATION_USAGE.md`.

## Changes Made

### 1. Main Game Loop (`src/main.cpp`)

#### Added Network State Variables
- `awaiting_player_network_id`: Tracks if we're waiting for a network ID grant
- `local_player`: Stores the local player entity once created (optional type)

#### Modified Player Creation Flow
**Before:** Player entity was created immediately at startup with hardcoded network component.

**After:** Player entity creation follows the proper network flow:
1. When hosting or joining, request a network ID via `network_system->request_network_id()`
2. Wait for the network ID grant in the game loop
3. Create the networked player entity with the granted ID
4. Add all required components (transform, rigidbody, sprite, gravity, jump, inventory, entity_state, player)
5. Add network sync components (network_position, network_rigidbody)
6. Send entity initialization to the network via `network_system->send_entity_init()`

#### Game Loop Protection
- Added check to ensure local player exists before processing game logic
- Shows "Waiting for network ID..." message while waiting for player creation
- Continues to process network updates even while waiting

### 2. Player Input System (`src/systems/player_input_system.cpp`)

#### Added Local Player Filtering
**Problem:** Input system would control ALL player entities, including remote players.

**Solution:** Added check for `network.is_local` component:
```cpp
if (g_conductor.has_component<network>(entity)) {
  auto &network_comp = g_conductor.get_component<network>(entity);
  if (!network_comp.is_local) {
    continue; // Skip remote players
  }
}
```

This ensures:
- Local player is controlled by keyboard input
- Remote players are NOT affected by local keyboard input
- Remote players are controlled by network updates only

### 3. Component Manager (`include/component_manager.hpp`)

#### Added `has_component<T>()` Method
New method to check if an entity has a specific component without throwing exceptions:
```cpp
template<typename T>
bool has_component(entity entity);
```

This is used by player_input_system to check if entities have the network component.

### 4. Component Array (`include/i_component_array.hpp`)

#### Added `has_data()` Method
Underlying implementation for `has_component`:
```cpp
template <typename T> bool has_data(entity ent);
```

Returns true if the entity has the component in the array.

### 5. Conductor (`include/conductor.hpp`)

#### Exposed `has_component<T>()` Method
Delegates to component_manager's implementation:
```cpp
template <typename T> bool has_component(entity entity);
```

### 6. Component Serialization (`include/component_serialization.hpp`)

#### Added New ComponentIDs
- `Player = 10`: For the player marker component
- `EntityState = 11`: For the entity_state component

These allow proper serialization/deserialization of these components over the network.

### 7. Network System (`src/systems/network_system.cpp`)

#### Extended `send_entity_init()`
Added serialization for:
- `player` component (marker)
- `entity_state` component (active state)
- `network_position` component (sync data)
- `network_rigidbody` component (sync data)

#### Extended `handle_entity_init()`
Added deserialization cases for:
- `ComponentID::Player`: Deserializes and adds player component
- `ComponentID::EntityState`: Deserializes and adds entity_state component
- `ComponentID::NetworkPosition`: Deserializes and adds network_position component
- `ComponentID::NetworkRigidbody`: Deserializes and adds network_rigidbody component

#### Improved Sprite Texture Loading
When receiving a remote entity with a sprite component:
```cpp
if (!spr.texture_name.empty()) {
  if (!spr.texture.loadFromFile(spr.texture_name)) {
    std::cerr << "Failed to load texture: " << spr.texture_name << std::endl;
  } else {
    spr.sprite_obj = sf::Sprite(spr.texture);
  }
}
```

This ensures remote players appear with the correct texture.

### 8. CMakeLists.txt

#### Added Missing Source Files
- `src/component_serialization.cpp`
- `src/systems/network_sync_system.cpp`

These were created in the previous implementation but not added to the build system.

## How It Works

### Host Flow
1. Press 'H' to start hosting
2. NetworkManager starts listening on port 27020
3. **Host allocates its own network ID directly** (using `NetworkManager::AllocateNetworkId()`)
4. Create networked player entity immediately with allocated ID
5. Add all components (including network sync components)
6. Host player is ready (no need to broadcast to self)
7. Future connecting clients will request their own IDs from the host

### Client Flow
1. Press 'J' to join localhost
2. NetworkManager connects to host at 127.0.0.1:27020
3. Request network ID from host
4. Wait for host to allocate and grant ID
5. Receive network ID grant
6. Create networked player entity with granted ID
7. Add all components (including network sync components)
8. Send entity initialization to host (which forwards to other clients)

### During Gameplay

#### Local Player (is_local = true)
- Physics system updates transform/rigidbody based on input
- network_sync_system copies transform → network_position, rigidbody → network_rigidbody
- network_system broadcasts network_position/network_rigidbody updates to network
- Render system renders at actual transform position

#### Remote Player (is_local = false)
- network_system receives network_position/network_rigidbody updates
- network_system interpolates between last and target positions (smooth movement)
- network_sync_system copies network_position → transform, network_rigidbody → rigidbody
- Render system renders at interpolated transform position

## Testing Checklist

- [x] Build succeeds without errors
- [x] No linter errors
- [ ] Host can start and see local player
- [ ] Client can join and see local player
- [ ] Host can see client's player
- [ ] Client can see host's player
- [ ] Multiple clients can see each other
- [ ] Player movement is smooth (interpolation working)
- [ ] Player textures load correctly on remote clients
- [ ] Physics (gravity, jumping) works correctly
- [ ] Collision detection works on both local and remote players

## Known Limitations

1. **Single Player Component**: The `player` component is used as a marker for ALL player entities (local and remote). If you want to distinguish YOUR player for UI purposes (e.g., highlighting), you'll need to track the local_player entity separately (as done in main.cpp).

2. **No Player Despawn**: Currently, there's no mechanism to remove a player entity when a client disconnects. This would need to be implemented separately.

3. **No Late Join Synchronization**: If a client joins after players already exist, they won't receive the existing player entities. A "full state synchronization" packet would be needed for this.

4. **Texture Loading**: Remote entities assume textures exist at the specified path. If a texture is missing, the sprite will be invisible.

## Future Improvements

1. **Player Disconnect Handling**: Detect disconnections and remove player entities
2. **Late Join State Sync**: Send all existing entities to newly connected clients
3. **Player Color/Customization**: Differentiate players visually
4. **Network Bandwidth Optimization**: Send updates at a lower rate (currently every frame)
5. **Prediction**: Add client-side prediction for smoother local player movement
6. **Authority Reconciliation**: Handle conflicts when both client and server modify same entity

## Related Documentation

- `NETWORKED_ENTITY_CREATION_USAGE.md` - Complete usage guide for networked entities
- `NETWORK_FLOW_DIAGRAM.md` - Visual flow diagrams
- `IMPLEMENTATION_SUMMARY.md` - Technical implementation details

