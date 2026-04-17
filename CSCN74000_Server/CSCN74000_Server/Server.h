#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <vector>

#include "Packet.h"
#include "ServerState.h"

// Handles all server-side networking logic including
// accepting clients, processing requests, and sending responses.
class Server
{
public:
    Server();
    ~Server();

    // Initializes server, socket, and starts listening.
    bool Start();

    // Stops server and cleans up all resources.
    void Stop();

    // Main loop to accept and handle client requests.
    void Run();

private:
    // Initializes Winsock library.
    bool InitializeWinsock();

    // Creates the listening socket.
    bool CreateListenSocket();

    // Binds socket to server address and port.
    bool BindSocket();

    // Puts socket into listening mode.
    bool ListenForClient();

    // Accepts incoming client connection.
    bool AcceptClient();

    // Ensures full data reception.
    bool ReceiveAll(char* buffer, int totalBytes);

    // Ensures full data transmission.
    bool SendAll(const char* buffer, int totalBytes);

    // Receives a complete packet from client.
    bool ReceivePacket(Shared::Packet& outPacket);

    // Sends a packet to client.
    bool SendPacket(const Shared::Packet& packet);

    // Handles client verification logic.
    Shared::Packet HandleVerification(const Shared::Packet& packet);

    // Handles sensor data request.
    Shared::Packet HandleSensorRequest();

    // Handles client disconnect request.
    Shared::Packet HandleDisconnectRequest();

    // Handles telemetry file request.
    bool HandleTelemetryRequest();

    // Sends telemetry file in chunks.
    bool SendTelemetryFile();

    // Checks if client is verified before allowing requests.
    bool IsVerified() const;

    // Closes active client socket.
    void CloseClientSocket();

    // Closes listening socket.
    void CloseListenSocket();

    // Cleans up Winsock.
    void CleanupWinsock();

    // Updates server state.
    void SetState(Shared::ServerState newState);

    // Returns string representation of current state.
    std::string GetStateString() const;

private:
    WSADATA m_wsaData;            // Winsock data
    SOCKET m_listenSocket;        // Socket for listening to connections
    SOCKET m_clientSocket;        // Active client socket
    sockaddr_in m_serverAddress;  // Server address info
    Shared::ServerState m_state;  // Current server state
    bool m_isRunning;             // Server running flag
};