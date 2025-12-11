#include "network_manager.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <steam/isteamnetworkingsockets.h>
#include <steam/steamclientpublic.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/steamnetworkingtypes.h>
#include <string>

NetworkManager &NetworkManager::Get() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() { Shutdown(); }

bool NetworkManager::Init() {
    SteamDatagramErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
        std::cerr << "GameNetworkingSockets_Init failed.  " << errMsg
                  << std::endl;
        return false;
    }
    m_pInterface = SteamNetworkingSockets();
    return true;
}

void NetworkManager::Shutdown() {
    if (m_hListenSocket != k_HSteamListenSocket_Invalid) {
        m_pInterface->CloseListenSocket(m_hListenSocket);
        m_hListenSocket = k_HSteamListenSocket_Invalid;
    }
    if (m_hConnection != k_HSteamNetConnection_Invalid) {
        m_pInterface->CloseConnection(m_hConnection, 0, "Shutdown", false);
        m_hConnection = k_HSteamNetConnection_Invalid;
    }
    GameNetworkingSockets_Kill();
}

void NetworkManager::SteamNetConnectionStatusChangedCallback(
    SteamNetConnectionStatusChangedCallback_t *pInfo) {
    NetworkManager::Get().OnConnectionStatusChanged(pInfo);
}

void NetworkManager::OnConnectionStatusChanged(
    SteamNetConnectionStatusChangedCallback_t *pInfo) {
    switch (pInfo->m_info.m_eState) {
    case k_ESteamNetworkingConnectionState_None:
        // NOTE: We will get callbacks here when we destroy connections.  You
        // can ignore these.
        break;

    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        // Ignore if they were not previously connected.  (If they disconnected
        // before we accepted the connection.)
        if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
            // Locate the client.  Note that it should have been found, because
            // this is the only path through which the connection could have
            // been accepted!
            // FIXME: Handle disconnection
            std::cout << "Connection closed/problem." << std::endl;
        } else {
            assert(pInfo->m_eOldState ==
                   k_ESteamNetworkingConnectionState_Connecting);
        }

        // Clean up the connection.  This is important!
        // The connection is "closed" in the network sense, but
        // it has not been destroyed.  We must close it on our end, too
        // to finish the destruction.  If we don't do this, we'll leak
        // memory on our end, and the socket will remain open.
        //
        // We can't just call CloseConnection() because that sends a message
        // to the peer, and we know the peer is gone.  (Or we just decided
        // to hang up on them.)
        //
        // If the connection was initiated by the peer, we can just release
        // the handle.
        m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);

        if (m_hConnection == pInfo->m_hConn) {
            m_hConnection = k_HSteamNetConnection_Invalid;
            m_connected = false;
        }

        // Remove from client map if host
        if (m_isHost) {
            m_clientConnections.erase(pInfo->m_hConn);
        }

        break;

    case k_ESteamNetworkingConnectionState_Connecting:
        // We will get this callback when they start connecting.
        // We must accept the connection.
        if (m_isHost) {
            if (m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK) {
                m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr,
                                              false);
                std::cout
                    << "Can't accept connection.  (It was already closed?)"
                    << std::endl;
                break;
            }
            // Assign a player ID or something
            std::cout << "Accepted connection " << pInfo->m_hConn << std::endl;
            m_clientConnections[pInfo->m_hConn] = 0; // Placeholder ID
        }
        break;

    case k_ESteamNetworkingConnectionState_Connected:
        std::cout << "Connected to remote host." << std::endl;
        m_connected = true;
        break;

    default:
        // Silences -Wswitch
        break;
    }
}

bool NetworkManager::StartHost(uint16_t port) {
    m_isHost = true;
    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    serverAddr.m_port = port;

    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
               (void *)SteamNetConnectionStatusChangedCallback);

    m_hListenSocket = m_pInterface->CreateListenSocketIP(serverAddr, 1, &opt);
    if (m_hListenSocket == k_HSteamListenSocket_Invalid) {
        std::cerr << "Failed to listen on port " << port << std::endl;
        return false;
    }

    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

bool NetworkManager::Connect(const std::string &address) {
    m_isHost = false;
    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    if (!serverAddr.ParseString(address.c_str())) {
        std::cerr << "Invalid address: " << address << std::endl;
        return false;
    }

    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
               (void *)SteamNetConnectionStatusChangedCallback);

    m_hConnection = m_pInterface->ConnectByIPAddress(serverAddr, 1, &opt);
    if (m_hConnection == k_HSteamNetConnection_Invalid) {
        std::cerr << "Failed to create connection." << std::endl;
        return false;
    }

    return true;
}

void NetworkManager::Update() {
    PollIncomingMessages();
    m_pInterface->RunCallbacks();
}

void NetworkManager::PollIncomingMessages() {
    ISteamNetworkingMessage *pIncomingMsgs[32];

    if (m_isHost) {
        // Host: poll messages from all client connections
        for (auto const &[conn, id] : m_clientConnections) {
            int numMsgs = m_pInterface->ReceiveMessagesOnConnection(
                conn, pIncomingMsgs, 32);
            if (numMsgs < 0) {
                std::cerr << "Error checking for messages on connection."
                          << std::endl;
                continue;
            }

            for (int i = 0; i < numMsgs; i++) {
                if (m_packetCallback) {
                    m_packetCallback(pIncomingMsgs[i]->m_conn,
                                     pIncomingMsgs[i]->m_pData,
                                     pIncomingMsgs[i]->m_cbSize);
                }
                pIncomingMsgs[i]->Release();
            }
        }
    } else {
        // Client: poll messages from server connection
        if (m_hConnection != k_HSteamNetConnection_Invalid) {
            int numMsgs = m_pInterface->ReceiveMessagesOnConnection(
                m_hConnection, pIncomingMsgs, 32);
            if (numMsgs < 0) {
                std::cerr << "Error checking for messages on connection."
                          << std::endl;
                return;
            }

            for (int i = 0; i < numMsgs; i++) {
                if (m_packetCallback) {
                    m_packetCallback(pIncomingMsgs[i]->m_conn,
                                     pIncomingMsgs[i]->m_pData,
                                     pIncomingMsgs[i]->m_cbSize);
                }
                pIncomingMsgs[i]->Release();
            }
        }
    }
}

void NetworkManager::BroadcastPacket(const void *data, size_t size,
                                     int nSendFlags) {
    if (!m_isHost)
        return;

    for (auto const &[conn, id] : m_clientConnections) {
        m_pInterface->SendMessageToConnection(conn, data, size, nSendFlags,
                                              nullptr);
    }
}

void NetworkManager::SendPacketToServer(const void *data, size_t size,
                                        int nSendFlags) {
    if (m_isHost || m_hConnection == k_HSteamNetConnection_Invalid)
        return;
    m_pInterface->SendMessageToConnection(m_hConnection, data, size, nSendFlags,
                                          nullptr);
}

uint32_t NetworkManager::AllocateNetworkId() {
    if (!m_isHost)
        return 0; // Invalid ID
    return m_nextNetworkId++;
}

void NetworkManager::RegisterNetworkEntity(uint32_t netId, entity ent) {
    m_networkIdToEntity[netId] = ent;
}

entity NetworkManager::GetEntityByNetworkId(uint32_t netId) const {
    auto it = m_networkIdToEntity.find(netId);
    if (it != m_networkIdToEntity.end()) {
        return it->second;
    }
    return 0; // Invalid entity
}

HSteamNetConnection
NetworkManager::GetConnectionByNetworkId(uint32_t netId) const {
    auto it = m_networkIdToConnection.find(netId);
    if (it != m_networkIdToConnection.end()) {
        return it->second;
    }
    return k_HSteamNetConnection_Invalid;
}

void NetworkManager::SendToConnection(HSteamNetConnection conn,
                                      const void *data, size_t size,
                                      int nSendFlags) {
    if (conn == k_HSteamNetConnection_Invalid)
        return;
    m_pInterface->SendMessageToConnection(conn, data, size, nSendFlags,
                                          nullptr);
}