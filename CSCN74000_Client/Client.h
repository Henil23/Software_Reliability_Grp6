#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <vector>

#include "Packet.h"
#include "SensorData.h"

// Handles all client-side networking operations such as
// connecting to the server, sending requests, and receiving responses.
class Client
{
public:
    Client();
    ~Client();

    // Initializes Winsock and creates the client socket.
    bool Start();

    // Closes socket and cleans up Winsock resources.
    void Stop();

    // Connects the client to the server using the configured IP and port.
    bool ConnectToServer();

    // Runs the main client menu loop.
    void Run();

private:
    // Starts the Winsock library required for socket programming on Windows.
    bool InitializeWinsock();

    // Creates a TCP socket for communication with the server.
    bool CreateSocket();

    // Sends all bytes in the buffer, even if send() sends only part of it.
    bool SendAll(const char* buffer, int totalBytes);

    // Receives exactly totalBytes from the server.
    bool ReceiveAll(char* buffer, int totalBytes);

    // Serializes and sends a packet to the server.
    bool SendPacket(const Shared::Packet& packet);

    // Receives and reconstructs a full packet from the server.
    bool ReceivePacket(Shared::Packet& packet);

    // Builds a packet to request client verification.
    Shared::Packet BuildVerifyRequestPacket() const;

    // Builds a packet to request current sensor data.
    Shared::Packet BuildSensorRequestPacket() const;

    // Builds a packet to request the telemetry file.
    Shared::Packet BuildTelemetryRequestPacket() const;

    // Builds a packet to request disconnection from the server.
    Shared::Packet BuildDisconnectRequestPacket() const;

    // Builds an intentionally invalid packet for testing error handling.
    Shared::Packet BuildInvalidCommandPacket() const;
    
    // Sends verification request and handles the server response.
    void HandleVerifyRequest();

    // Sends sensor request and displays returned sensor data.
    void HandleSensorRequest();

    // Sends telemetry request and receives telemetry file data.
    void HandleTelemetryRequest();

    // Sends an invalid command to test server validation.
    void HandleInvalidCommandTest();

    // Sends disconnect request and returns true if disconnection succeeds.
    bool HandleDisconnectRequest();

    // Receives telemetry data in chunks and saves it to a file.
    bool ReceiveTelemetryFile();

    // Displays all received sensor data to the console.
    void DisplaySensors(const std::vector<Shared::SensorData>& sensors, std::uint64_t packetTimestamp) const;

    // Reads the user's menu choice safely.
    int ReadMenuChoice() const;

private:
    WSADATA m_wsaData; // Stores Winsock startup information.
    SOCKET m_socket;   // Client socket used for server communication.
};
