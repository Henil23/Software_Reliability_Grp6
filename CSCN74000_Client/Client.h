#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <vector>

#include "Packet.h"
#include "SensorData.h"

class Client
{
public:
    Client();
    ~Client();

    bool Start();
    void Stop();
    bool ConnectToServer();
    void Run();

private:
    bool InitializeWinsock();
    bool CreateSocket();

    bool SendAll(const char* buffer, int totalBytes);
    bool ReceiveAll(char* buffer, int totalBytes);

    bool SendPacket(const Shared::Packet& packet);
    bool ReceivePacket(Shared::Packet& packet);

    Shared::Packet BuildVerifyRequestPacket() const;
    Shared::Packet BuildSensorRequestPacket() const;
    Shared::Packet BuildTelemetryRequestPacket() const;
    Shared::Packet BuildDisconnectRequestPacket() const;
    Shared::Packet BuildInvalidCommandPacket() const;
    

    void HandleVerifyRequest();
    void HandleSensorRequest();
    void HandleTelemetryRequest();
    void HandleInvalidCommandTest();
    bool HandleDisconnectRequest();

    bool ReceiveTelemetryFile();

    void DisplaySensors(const std::vector<Shared::SensorData>& sensors, std::uint64_t packetTimestamp) const;
    int ReadMenuChoice() const;

private:
    WSADATA m_wsaData;
    SOCKET m_socket;
};