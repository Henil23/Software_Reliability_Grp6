#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Client.h"

#include <fstream>
#include <iostream>
#include <limits>

#include "Constants.h"
#include "Serialization.h"
#include "Logger.h"

#pragma comment(lib, "Ws2_32.lib")

Client::Client()
    : m_socket(INVALID_SOCKET)
{
    ZeroMemory(&m_wsaData, sizeof(m_wsaData));
}

Client::~Client()
{
    Stop();
}

bool Client::Start()
{
    if (!InitializeWinsock())
    {
        std::cout << "WSAStartup failed\n";
        return false;
    }

    if (!CreateSocket())
    {
        std::cout << "Socket creation failed\n";
        WSACleanup();
        return false;
    }

    return true;
}

void Client::Stop()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    WSACleanup();
}

bool Client::ConnectToServer()
{
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(Shared::DEFAULT_SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) != 1)
    {
        std::cout << "Invalid server address\n";
        return false;
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed\n";
        return false;
    }

    Shared::Logger::LogEvent(
        Shared::CLIENT_LOG_FILE,
        "Connected to server"
    );

    std::cout << "Connected to server\n";
    return true;
}

void Client::Run()
{
    while (true)
    {
        int choice = ReadMenuChoice();

        if (choice == 1)
        {
            HandleVerifyRequest();
        }
        else if (choice == 2)
        {
            HandleSensorRequest();
        }
        else if (choice == 3)
        {
            HandleTelemetryRequest();
        }
        else if (choice == 4)
        {
            Shared::Logger::LogEvent(
                Shared::CLIENT_LOG_FILE,
                "Client exiting"
            );
            break;
        }
        else
        {
            std::cout << "Invalid choice\n";
        }
    }
}

bool Client::InitializeWinsock()
{
    return WSAStartup(MAKEWORD(2, 2), &m_wsaData) == 0;
}

bool Client::CreateSocket()
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return m_socket != INVALID_SOCKET;
}

bool Client::SendAll(const char* buffer, int totalBytes)
{
    int bytesSent = 0;

    while (bytesSent < totalBytes)
    {
        int result = send(m_socket, buffer + bytesSent, totalBytes - bytesSent, 0);

        if (result == SOCKET_ERROR)
        {
            return false;
        }

        bytesSent += result;
    }

    return true;
}

bool Client::ReceiveAll(char* buffer, int totalBytes)
{
    int bytesReceived = 0;

    while (bytesReceived < totalBytes)
    {
        int result = recv(m_socket, buffer + bytesReceived, totalBytes - bytesReceived, 0);

        if (result <= 0)
        {
            return false;
        }

        bytesReceived += result;
    }

    return true;
}

bool Client::SendPacket(const Shared::Packet& packet)
{
    std::vector<std::uint8_t> buffer;

    if (!Shared::Serialization::SerializePacket(packet, buffer))
    {
        return false;
    }

    if (!SendAll(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size())))
    {
        return false;
    }

    Shared::Logger::LogPacket(
        Shared::CLIENT_LOG_FILE,
        packet,
        Shared::LogDirection::SENT
    );

    return true;
}

bool Client::ReceivePacket(Shared::Packet& packet)
{
    const std::size_t headerSize = Shared::Serialization::GetPacketHeaderSize();
    std::vector<std::uint8_t> headerBuffer(headerSize);

    if (!ReceiveAll(reinterpret_cast<char*>(headerBuffer.data()), static_cast<int>(headerBuffer.size())))
    {
        return false;
    }

    if (!Shared::Serialization::DeserializePacketHeader(headerBuffer, packet.header))
    {
        return false;
    }

    if (!Shared::Serialization::ValidatePayloadSize(packet.header.payloadSize))
    {
        return false;
    }

    packet.payload.clear();

    if (packet.header.payloadSize > 0)
    {
        packet.payload.resize(packet.header.payloadSize);

        if (!ReceiveAll(reinterpret_cast<char*>(packet.payload.data()), static_cast<int>(packet.payload.size())))
        {
            return false;
        }
    }

    if (!packet.IsValid())
    {
        return false;
    }

    Shared::Logger::LogPacket(
        Shared::CLIENT_LOG_FILE,
        packet,
        Shared::LogDirection::RECEIVED
    );

    return true;
}

Shared::Packet Client::BuildVerifyRequestPacket() const
{
    Shared::Packet packet;
    packet.header.type = Shared::PacketType::VERIFY_REQUEST;
    packet.header.timestamp = Shared::Serialization::GetCurrentTimestamp();
    packet.header.sensorCount = 0;
    packet.header.status = Shared::StatusCode::SUCCESS;

    if (!Shared::Serialization::BuildTextPayload(Shared::EXPECTED_VERIFICATION_TOKEN, packet.payload))
    {
        packet.payload.clear();
    }

    packet.UpdatePayloadSize();
    return packet;
}

Shared::Packet Client::BuildSensorRequestPacket() const
{
    Shared::Packet packet;
    packet.header.type = Shared::PacketType::SENSOR_REQUEST;
    packet.header.timestamp = Shared::Serialization::GetCurrentTimestamp();
    packet.header.sensorCount = 0;
    packet.header.status = Shared::StatusCode::SUCCESS;
    packet.UpdatePayloadSize();

    return packet;
}

Shared::Packet Client::BuildTelemetryRequestPacket() const
{
    Shared::Packet packet;
    packet.header.type = Shared::PacketType::TELEMETRY_REQUEST;
    packet.header.timestamp = Shared::Serialization::GetCurrentTimestamp();
    packet.header.sensorCount = 0;
    packet.header.status = Shared::StatusCode::SUCCESS;
    packet.UpdatePayloadSize();

    return packet;
}

void Client::HandleVerifyRequest()
{
    Shared::Packet request = BuildVerifyRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send verify request\n";
        return;
    }

    Shared::Packet response;
    if (!ReceivePacket(response))
    {
        std::cout << "Failed to receive verify response\n";
        return;
    }

    std::string message = Shared::Serialization::ExtractTextPayload(response.payload);

    std::cout << "Server Response: " << message << "\n";
    std::cout << "Status: " << static_cast<std::uint32_t>(response.header.status) << "\n";
}

void Client::HandleSensorRequest()
{
    Shared::Packet request = BuildSensorRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send sensor request\n";
        return;
    }

    Shared::Packet response;
    if (!ReceivePacket(response))
    {
        std::cout << "Failed to receive sensor response\n";
        return;
    }

    if (response.header.status != Shared::StatusCode::SUCCESS)
    {
        std::cout << "Error: ";

        if (response.header.status == Shared::StatusCode::NOT_VERIFIED)
        {
            std::cout << "Not Verified\n";
        }
        else
        {
            std::cout << "Request Failed\n";
        }

        return;
    }

    std::vector<Shared::SensorData> sensors;
    if (!Shared::Serialization::DeserializeSensorDataList(response.payload, sensors))
    {
        std::cout << "Failed to decode sensor data\n";
        return;
    }

    DisplaySensors(sensors, response.header.timestamp);
}

void Client::HandleTelemetryRequest()
{
    Shared::Packet request = BuildTelemetryRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send telemetry request\n";
        return;
    }

    if (!ReceiveTelemetryFile())
    {
        std::cout << "Telemetry transfer failed\n";
    }
}

bool Client::ReceiveTelemetryFile()
{
    std::ofstream outFile(Shared::RECEIVED_TELEMETRY_FILE_PATH, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open())
    {
        std::cout << "Failed to create output telemetry file\n";
        return false;
    }

    std::size_t totalBytes = 0;

    while (true)
    {
        Shared::Packet response;

        if (!ReceivePacket(response))
        {
            outFile.close();
            return false;
        }

        if (response.header.type == Shared::PacketType::ERROR_PACKET)
        {
            std::string message = Shared::Serialization::ExtractTextPayload(response.payload);

            std::cout << "Telemetry Error: ";
            if (!message.empty())
            {
                std::cout << message << "\n";
            }
            else if (response.header.status == Shared::StatusCode::NOT_VERIFIED)
            {
                std::cout << "Not Verified\n";
            }
            else
            {
                std::cout << "Request Failed\n";
            }

            outFile.close();
            return false;
        }

        if (response.header.type == Shared::PacketType::TELEMETRY_CHUNK)
        {
            if (!response.payload.empty())
            {
                outFile.write(
                    reinterpret_cast<const char*>(response.payload.data()),
                    static_cast<std::streamsize>(response.payload.size())
                );

                if (!outFile.good())
                {
                    outFile.close();
                    return false;
                }

                totalBytes += response.payload.size();
            }
        }
        else if (response.header.type == Shared::PacketType::TELEMETRY_COMPLETE)
        {
            outFile.close();

            std::string message = Shared::Serialization::ExtractTextPayload(response.payload);
            if (message.empty())
            {
                message = "Telemetry transfer complete";
            }

            std::cout << message << "\n";
            std::cout << "Saved to: " << Shared::RECEIVED_TELEMETRY_FILE_PATH << "\n";
            std::cout << "Bytes received: " << totalBytes << "\n";

            Shared::Logger::LogEvent(
                Shared::CLIENT_LOG_FILE,
                "Telemetry transfer complete"
            );

            return true;
        }
        else
        {
            outFile.close();
            std::cout << "Unexpected packet received during telemetry transfer\n";
            return false;
        }
    }
}

void Client::DisplaySensors(const std::vector<Shared::SensorData>& sensors, std::uint64_t packetTimestamp) const
{
    std::cout << "\n--- Sensor Data ---\n";
    std::cout << "Packet timestamp: " << packetTimestamp << "\n";

    for (const auto& sensor : sensors)
    {
        std::cout << sensor.name << " : " << sensor.value << " " << sensor.unit << "\n";
    }
}

int Client::ReadMenuChoice() const
{
    int choice = 0;

    std::cout << "\n1. Verify Connection\n2. Request Sensor Data\n3. Request Telemetry File\n4. Exit\nChoice: ";
    std::cin >> choice;

    if (std::cin.fail())
    {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        return -1;
    }

    return choice;
}