# Networked Entity Creation - Flow Diagrams

## Complete Entity Creation Flow

```mermaid
sequenceDiagram
    participant C as Client
    participant NS as network_system
    participant NM as NetworkManager
    participant H as Host
    participant OC as Other Clients

    Note over C: User wants to create entity
    C->>NS: request_network_id()
    NS->>NM: SendPacketToServer(ReserveNetworkIDRequest)
    NM->>H: [Network] ReserveNetworkIDRequest
    
    Note over H: Receives request
    H->>NS: handle_packet()
    NS->>NS: handle_reserve_id_request()
    NS->>NM: AllocateNetworkId()
    NM-->>NS: new_id (e.g., 42)
    
    Note over H: Broadcast reservation
    NS->>NM: BroadcastPacket(NetworkIDReserved)
    NM->>OC: [Network] NetworkIDReserved(42)
    
    Note over H: Grant to requester
    NS->>NM: SendToConnection(NetworkIDGranted)
    NM->>C: [Network] NetworkIDGranted(42)
    
    Note over C: Receives grant
    C->>NS: handle_packet()
    NS->>NS: handle_network_id_granted()
    NS->>NS: m_pendingGrantedId = 42
    
    Note over C: Check in game loop
    C->>NS: has_pending_granted_id()?
    NS-->>C: true
    C->>NS: get_pending_granted_id()
    NS-->>C: 42
    C->>NS: clear_pending_granted_id()
    
    Note over C: Create entity
    C->>C: create_networked_entity(42, true)
    C->>C: add_component<transform>()
    C->>C: add_component<rigidbody>()
    C->>C: add_component<sprite>()
    C->>C: add_component<network_position>()
    C->>C: add_component<network_rigidbody>()
    
    Note over C: Send initialization
    C->>NS: send_entity_init(entity)
    NS->>NS: Serialize all components
    NS->>NM: SendPacketToServer(EntityInitPacket)
    NM->>H: [Network] EntityInitPacket
    
    Note over H: Create matching entity
    H->>NS: handle_packet()
    NS->>NS: handle_entity_init()
    NS->>C: create_networked_entity(42, false)
    NS->>C: Deserialize and add components
    
    Note over H: Forward to others
    H->>NM: BroadcastPacket(EntityInitPacket)
    NM->>OC: [Network] EntityInitPacket
    
    Note over OC: Create matching entity
    OC->>NS: handle_packet()
    NS->>NS: handle_entity_init()
    NS->>OC: create_networked_entity(42, false)
    NS->>OC: Deserialize and add components
    
    Note over C,OC: Entity now exists on all clients!
```

## Network Synchronization (Every Frame)

```mermaid
sequenceDiagram
    participant C as Client (Local)
    participant NSS as network_sync_system
    participant NS as network_system
    participant NM as NetworkManager
    participant H as Host
    participant R as Remote Client

    Note over C: Game loop frame starts
    
    Note over C: Physics/collision updates
    C->>C: Update transform/rigidbody
    
    Note over C: Sync to network components
    C->>NSS: update(dt)
    NSS->>NSS: sync_local_to_network()
    NSS->>C: transform → network_position
    NSS->>C: rigidbody → network_rigidbody
    
    Note over C: Send updates
    C->>NS: update(dt)
    NS->>NS: broadcast_network_position()
    NS->>NS: broadcast_network_rigidbody()
    NS->>NM: SendPacketToServer(NetworkPositionUpdate)
    NS->>NM: SendPacketToServer(NetworkRigidbodyUpdate)
    NM->>H: [Network] Updates
    
    Note over H: Forward to remotes
    H->>NM: BroadcastPacket(NetworkPositionUpdate)
    H->>NM: BroadcastPacket(NetworkRigidbodyUpdate)
    NM->>R: [Network] Updates
    
    Note over R: Receive updates
    R->>NS: handle_packet()
    NS->>NS: handle_network_position_update()
    NS->>R: Update network_position.target_x/y
    NS->>NS: handle_network_rigidbody_update()
    NS->>R: Update network_rigidbody.target_*
    
    Note over R: Interpolate
    R->>NS: update(dt)
    NS->>NS: interpolate_network_components()
    NS->>R: Smooth transition to targets
    
    Note over R: Sync to gameplay
    R->>NSS: update(dt)
    NSS->>NSS: sync_network_to_local()
    NSS->>R: network_position → transform
    NSS->>R: network_rigidbody → rigidbody
    
    Note over R: Render updated position
    R->>R: Render entity at smooth position
```

## Component Synchronization Flow

```mermaid
flowchart TD
    Start[Game Frame Starts]
    
    Start --> CheckLocal{Is entity<br/>is_local?}
    
    CheckLocal -->|Yes - Local Entity| LocalPath[Local Authority Path]
    CheckLocal -->|No - Remote Entity| RemotePath[Remote Authority Path]
    
    LocalPath --> Physics[Physics/Input Systems<br/>Update transform/rigidbody]
    Physics --> SyncToNet[network_sync_system:<br/>Copy to network_position/rigidbody]
    SyncToNet --> Broadcast[network_system:<br/>Broadcast updates to network]
    Broadcast --> Render1[Render at<br/>local position]
    
    RemotePath --> Receive[network_system:<br/>Receive network updates]
    Receive --> UpdateTargets[Update target_x/y<br/>in network components]
    UpdateTargets --> Interpolate[network_system:<br/>Interpolate between last and target]
    Interpolate --> SyncToLocal[network_sync_system:<br/>Copy to transform/rigidbody]
    SyncToLocal --> Render2[Render at<br/>interpolated position]
    
    Render1 --> End[Frame Complete]
    Render2 --> End
```

## Ownership Transfer Flow

```mermaid
sequenceDiagram
    participant C1 as Client 1 (Current Owner)
    participant NS as network_system
    participant H as Host
    participant C2 as Client 2 (New Owner)
    participant OC as Other Clients

    Note over C1: Wants to transfer ownership
    C1->>NS: transfer_ownership(entity, player_id_2)
    NS->>NS: Get network.id from entity
    NS->>H: Send OwnershipTransferPacket
    
    Note over H: Receives transfer request
    H->>NS: handle_packet()
    NS->>NS: handle_ownership_transfer()
    NS->>H: Set entity.network.is_local = false
    
    Note over H: Broadcast to all clients
    H->>C1: Broadcast OwnershipTransferPacket
    H->>C2: Broadcast OwnershipTransferPacket
    H->>OC: Broadcast OwnershipTransferPacket
    
    Note over C1: Receives confirmation
    C1->>NS: handle_packet()
    NS->>NS: handle_ownership_transfer()
    NS->>C1: Set entity.network.is_local = false
    Note over C1: No longer has authority
    
    Note over C2: Receives ownership
    C2->>NS: handle_packet()
    NS->>NS: handle_ownership_transfer()
    NS->>NS: Check if new_owner_id == my_id
    NS->>C2: Set entity.network.is_local = true
    Note over C2: Now has authority!
    
    Note over OC: Receives update
    OC->>NS: handle_packet()
    NS->>NS: handle_ownership_transfer()
    NS->>OC: Set entity.network.is_local = false
    Note over OC: Remains remote
```

## System Architecture

```mermaid
flowchart TB
    subgraph Client
        Input[Input System]
        Physics[Physics System]
        Collision[Collision System]
        
        Input --> Transform1[transform]
        Physics --> Rigidbody1[rigidbody]
        Collision --> Transform1
        
        Transform1 --> NSS1[network_sync_system]
        Rigidbody1 --> NSS1
        
        NSS1 --> NetPos1[network_position]
        NSS1 --> NetRB1[network_rigidbody]
        
        NetPos1 --> NS1[network_system]
        NetRB1 --> NS1
        
        NS1 --> NM1[NetworkManager]
    end
    
    subgraph Network
        NM1 <-->|Packets| NM2[NetworkManager]
    end
    
    subgraph Host
        NM2 --> NS2[network_system]
        
        NS2 --> NetPos2[network_position]
        NS2 --> NetRB2[network_rigidbody]
        
        NetPos2 --> NSS2[network_sync_system]
        NetRB2 --> NSS2
        
        NSS2 --> Transform2[transform]
        NSS2 --> Rigidbody2[rigidbody]
        
        Transform2 --> Render[Render System]
        Rigidbody2 --> Render
    end
    
    style NetPos1 fill:#f9f,stroke:#333
    style NetRB1 fill:#f9f,stroke:#333
    style NetPos2 fill:#9ff,stroke:#333
    style NetRB2 fill:#9ff,stroke:#333
```

## Legend

- **Pink components**: Network components on local entity (sending updates)
- **Cyan components**: Network components on remote entity (receiving updates)
- **Solid arrows**: Data flow within a single machine
- **Dashed arrows**: Network communication between machines

## Key Timing Points

1. **ID Request**: Can happen anytime, typically on user action (spawn button)
2. **ID Grant**: Asynchronous, check `has_pending_granted_id()` every frame
3. **Entity Init**: Immediately after receiving ID grant
4. **Position Updates**: Every frame for local entities
5. **Interpolation**: Every frame for remote entities
6. **Sync**: Twice per frame (before and after network updates)

