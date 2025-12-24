#pragma once

#include "entity.hpp"
#include "packets.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>
#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/steamnetworkingtypes.h>

class NetworkManager {
public:
  static NetworkManager &Get();

  bool Init();
  void Shutdown();
  void Update();

  // Host functions
  bool StartHost(uint16_t port);
  void BroadcastPacket(const void *data, size_t size,
                       int nSendFlags = k_nSteamNetworkingSend_Reliable);

  // Client functions
  bool Connect(const std::string &address);
  void SendPacketToServer(const void *data, size_t size,
                          int nSendFlags = k_nSteamNetworkingSend_Reliable);

  bool IsHost() const { return m_isHost; }
  bool IsConnected() const { return m_connected; }
  uint32_t GetLocalPlayerId() const { return m_localPlayerId; }

  // Network ID management
  uint32_t AllocateNetworkId(); // Host only
  void RegisterNetworkEntity(uint32_t netId, entity ent);
  void UnregisterNetworkEntity(entity ent); // NEW: Unregister entity when destroyed
  entity GetEntityByNetworkId(uint32_t netId) const;
  HSteamNetConnection GetConnectionByNetworkId(uint32_t netId) const;
  void SendToConnection(HSteamNetConnection conn, const void *data, size_t size,
                        int nSendFlags = k_nSteamNetworkingSend_Reliable);

  // Callbacks
  using PacketReceivedCallback =
      std::function<void(HSteamNetConnection, const void *, size_t)>;
  void SetPacketCallback(PacketReceivedCallback callback) {
    m_packetCallback = callback;
  }

private:
  NetworkManager();
  ~NetworkManager();

  static void SteamNetConnectionStatusChangedCallback(
      SteamNetConnectionStatusChangedCallback_t *pInfo);

  void
  OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);
  void PollIncomingMessages();
  void PollConnectionStateChanges();

  ISteamNetworkingSockets *m_pInterface = nullptr;
  HSteamListenSocket m_hListenSocket = k_HSteamListenSocket_Invalid;
  HSteamNetConnection m_hConnection =
      k_HSteamNetConnection_Invalid; // For client: connection to server. For
                                     // host: unused (host manages multiple
                                     // connections)

  std::map<HSteamNetConnection, uint32_t>
      m_clientConnections; // Host: map connection to player ID

  bool m_isHost = false;
  bool m_connected = false;
  uint32_t m_localPlayerId = 0;

  PacketReceivedCallback m_packetCallback;

  // Network ID management
  uint32_t m_nextNetworkId = 1; // Start from 1, 0 is invalid
  std::map<uint32_t, entity> m_networkIdToEntity;
  std::map<uint32_t, HSteamNetConnection> m_networkIdToConnection;
};
