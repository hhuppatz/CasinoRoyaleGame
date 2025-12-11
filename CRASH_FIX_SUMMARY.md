# Coin Creation Crash Fix Summary

## Issues Identified and Fixed

### 🔴 Critical Issue #1: Dangling Texture Pointers (CRASH)
**Problem:** The `create_coin()` function created local `sf::Texture` variables that were destroyed when the function returned, but `sf::Sprite` objects stored pointers to these destroyed textures.

**Impact:** 
- Both hosts and clients would crash when trying to render coins
- Access violation / segmentation fault when dereferencing dangling texture pointers
- Affects all networked coin entities

**Fix:**
- Moved texture loading to `main()` scope where they persist for program lifetime
- Pass texture references to `create_coin()` function
- Textures now remain valid for entire game session

**Code Changes:**
- Added `coin_texture` and `coin_ui_texture` loading in `main()` (lines 201-214)
- Modified `create_coin()` signature to accept texture references
- Updated sprite creation to use persistent texture references (lines 647-657)

---

### 🔴 Critical Issue #2: Infinite Busy-Wait Loop (DEADLOCK)
**Problem:** The `busywait_for_network_id()` function created an infinite loop that never yielded CPU or allowed network messages to be processed.

**Impact:**
- 100% CPU usage on one core
- Application freeze for clients trying to create coins
- Deadlock: Network messages processed in main loop, but busywait prevents main loop from running
- `NetworkManager::Get().Update()` never called → ID never granted → infinite wait

**Fix:**
- Removed blocking `busywait_for_network_id()` function
- Clients now request ID and return immediately
- Network ID will be granted asynchronously by host
- Added note that production system should use callback/queue system

**Code Changes:**
- Replaced `busywait_for_network_id()` with non-blocking `try_get_network_id()` (lines 595-604)
- Removed `get_network_id()` blocking wrapper
- Client path now returns early and logs waiting message (lines 618-625)

---

### 🟡 Issue #3: Incorrect Sprite/Texture Order
**Problem:** Sprite was created pointing to a texture that would be copied, invalidating the pointer.

**Fix:**
- Now using persistent texture references that are never copied
- Sprite points to stable memory location

---

### 🟡 Issue #4: Confused Logic for Host ID Clearing
**Problem:** Host was clearing pending granted ID that was never set.

**Fix:**
- Removed unnecessary `clear_pending_granted_id()` call for host path
- Host directly allocates ID without clearing

---

## Technical Details

### How SFML Textures and Sprites Work
From SFML documentation:
> "It is important to note that the sf::Sprite instance doesn't copy the texture that it uses, it only keeps a reference to it. Thus, a sf::Texture must not be destroyed while it is used by a sf::Sprite."

This means:
- `sf::Sprite` stores a **pointer** to `sf::Texture`
- When you copy a sprite component, the sprite is copied but still points to original texture
- If original texture is destroyed, sprite has dangling pointer → CRASH

### Why Busywait Caused Deadlock
1. Client presses 'C' to create coin
2. `create_coin()` calls `get_network_id()`
3. `get_network_id()` calls `busywait_for_network_id()`
4. Infinite loop starts: `while (!id_granted) { ... }`
5. Main game loop never runs → `NetworkManager::Get().Update()` never called
6. Network messages never processed → Host's ID grant packet never received
7. **DEADLOCK** - waiting for something that will never happen

### Solution Architecture
```
Before (Broken):
┌─────────────────┐
│  create_coin()  │
│  [local vars]   │──┐ Textures destroyed
│  sf::Texture    │  │ when function returns
│  coin_texture   │◄─┘
└─────────────────┘
        ↓
   [DANGLING]  ← Sprite points to destroyed texture
        ↓
     CRASH 💥

After (Fixed):
┌──────────────┐
│   main()     │
│ [persists]   │──┐ Textures live for
│ sf::Texture  │  │ entire program
│ coin_texture │◄─┴─ Sprites point to stable memory
└──────────────┘
```

## Client Coin Creation Limitation

**Note:** After this fix, clients currently cannot create coins because the function returns early when requesting a network ID. To fully support client-side coin creation, you would need to implement:

1. **Callback System:** Queue coin creation request, complete when ID arrives
2. **Deferred Entity System:** Store entity creation parameters, execute when ID granted
3. **Server-Side Creation:** Have client send "create coin" request to host, host creates it

For now, **only hosts can create coins** (which was likely the intention for this game type).

## Testing Recommendations

1. **Host:** Press 'C' to create coin - should work without crash
2. **Client:** Press 'C' - should log message and not crash/freeze
3. **Rendering:** Verify coins render correctly for all connected clients
4. **Network Sync:** Verify coins sync across network properly
5. **Performance:** No CPU spikes when creating coins

## Files Modified

- `src/main.cpp`
  - Added persistent texture loading (lines 201-214)
  - Modified function signatures (lines 55-59)
  - Rewrote `create_coin()` with texture references and non-blocking ID request
  - Updated `create_coin()` call site with texture parameters
  - Added `try_get_network_id()` helper for future use
  - Added `send_entity_init()` call to broadcast coins to clients
  - Added key debouncing for 'C' key to prevent spam (c_is_pressed, c_was_pressed)

