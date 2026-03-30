#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <vector>

#include "Packet.h"
#include "ServerState.h"

class Server
{
public:
    Server();
    ~Server();

    bool Start();
    void Stop();
    void Run();

private:
    bool InitializeWinsock();
    bool CreateListenSocket();
    bool BindSocket();
    bool ListenForClient();
    bool AcceptClient();

    bool ReceiveAll(char* buffer, int totalBytes);
    bool SendAll(const char* buffer, int totalBytes);

    bool ReceivePacket(Shared::Packet& outPacket);
    bool SendPacket(const Shared::Packet& packet);

    Shared::Packet HandleVerification(const Shared::Packet& packet);
    Shared::Packet HandleSensorRequest();
    bool HandleTelemetryRequest();
    bool SendTelemetryFile();

    bool IsVerified() const;

    void CloseClientSocket();
    void CloseListenSocket();
    void CleanupWinsock();

    void SetState(Shared::ServerState newState);
    std::string GetStateString() const;

private:
    WSADATA m_wsaData;
    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    sockaddr_in m_serverAddress;
    Shared::ServerState m_state;
    bool m_isRunning;
};