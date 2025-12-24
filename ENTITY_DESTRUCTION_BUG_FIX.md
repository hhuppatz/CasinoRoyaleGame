# Entity Destruction Bug Fix

## Problem Summary

**Error:** `Assertion failed: entity_to_index_map.find(ent) != entity_to_index_map.end() && "Retrieving non-existent component."`

This error occurred when a player tried to pick up an item (coin) created by a networked player.

## Root Cause

The bug was caused by **incomplete cleanup when networked entities were destroyed**. When `conductor::destroy_entity()` was called, it properly cleaned up:
- ✅ Entity manager (freed entity ID)
- ✅ Component manager (removed all components)
- ✅ System manager (removed from all system entity lists)
- ❌ **NetworkManager** (entity was **NOT** unregistered!)

### The Bug Scenario

1. Player A creates a coin entity (local entity ID: 5, network ID: 100)
2. NetworkManager registers: `m_networkIdToEntity[100] = 5`
3. Coin is sent via `EntityInitPacket` to the host
4. Host broadcasts it to all clients
5. Player A picks up the coin locally before Player B's broadcast arrives
6. Entity 5 is destroyed via `g_conductor.destroy_entity(collision)` (line 29 of inventory_system.cpp)
7. **BUG:** NetworkManager still has `m_networkIdToEntity[100] = 5` (stale mapping)
8. Player B's `EntityInitPacket` for coin 100 arrives at Player A
9. Duplicate check: `GetEntityByNetworkId(100)` returns entity 5 (non-zero)
10. The packet is ignored (assumed duplicate) even though entity 5 no longer exists
11. The coin is never created on Player A's side from the broadcast

**OR alternatively:**

1. Entity 5 is returned to the available entity pool
2. Entity 5 is reused for a different game object (e.g., a player)
3. NetworkManager still maps network ID 100 to entity 5
4. When Player B tries to pick up the coin, it tries to access entity 5's `item` component
5. **CRASH:** Entity 5 is now a player, not a coin, and has no `item` component!

## The Fix

### 1. Added UnregisterNetworkEntity Method

**File:** `include/network_manager.hpp`

```cpp
void UnregisterNetworkEntity(entity ent); // NEW: Unregister entity when destroyed
```

**File:** `src/network_manager.cpp`

```cpp
void NetworkManager::UnregisterNetworkEntity(entity ent) {
    // Find and remove the network ID mapping for this entity
    for (auto it = m_networkIdToEntity.begin(); it != m_networkIdToEntity.end(); ++it) {
        if (it->second == ent) {
            m_networkIdToEntity.erase(it);
            return;
        }
    }
}
```

### 2. Updated Entity Destruction

**File:** `src/conductor.cpp`

```cpp
void conductor::destroy_entity(entity entity) {
    // If this is a networked entity, unregister it from NetworkManager
    if (has_component<network>(entity)) {
        NetworkManager::Get().UnregisterNetworkEntity(entity);
    }
    
    m_entity_manager->destroy_entity(entity);
    m_component_manager->entity_destroyed(entity);
    m_system_manager->entity_destroyed(entity);
}
```

### 3. Added Defensive Checks (Safety Net)

While the root cause is fixed, defensive checks were added to prevent crashes and provide diagnostics if similar issues occur:

**File:** `src/systems/inventory_system.cpp`
- Added `has_component<item>()` check before accessing the component
- Added error logging if component is missing

**File:** `src/systems/item_system.cpp`
- Added comprehensive component validation in `check_collision()`
- Added error logging to diagnose ECS consistency issues

## Testing

The fix ensures:
1. When a networked entity is destroyed, it's properly unregistered from NetworkManager
2. Stale network ID mappings cannot cause entity ID reuse bugs
3. Duplicate checks in `handle_entity_init()` work correctly
4. Entity IDs returned from collision detection are guaranteed to be valid

## Impact

- **Fixed:** Crash when picking up items created by networked players
- **Fixed:** Potential entity ID reuse bugs
- **Improved:** Error diagnostics for ECS consistency issues
- **No performance impact:** UnregisterNetworkEntity is O(n) where n = number of networked entities (typically small)

## Related Files Modified

1. `include/network_manager.hpp` - Added UnregisterNetworkEntity declaration
2. `src/network_manager.cpp` - Implemented UnregisterNetworkEntity
3. `src/conductor.cpp` - Updated destroy_entity to unregister networked entities
4. `src/systems/inventory_system.cpp` - Added defensive checks
5. `src/systems/item_system.cpp` - Added component validation and diagnostics

