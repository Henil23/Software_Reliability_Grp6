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
    // Initialize Winsock data structure to zero.
    ZeroMemory(&m_wsaData, sizeof(m_wsaData));
}

Client::~Client()
{
    // Ensure all networking resources are released when object is destroyed.
    Stop();
}

bool Client::Start()
{
    // Start Winsock before using sockets.
    if (!InitializeWinsock())
    {
        std::cout << "WSAStartup failed\n";
        return false;
    }

    // Create the TCP socket used by the client.
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
    // Close the socket if it is currently open.
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    // Release Winsock resources.
    WSACleanup();
}

bool Client::ConnectToServer()
{
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(Shared::DEFAULT_SERVER_PORT);

    // Convert IP address string into binary form.
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) != 1)
    {
        std::cout << "Invalid server address\n";
        return false;
    }

    // Attempt to connect to the server.
    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
    {
        std::cout << "Connection failed\n";
        return false;
    }

    // Log successful connection.
    Shared::Logger::LogEvent(
        Shared::CLIENT_LOG_FILE,
        "Connected to server"
    );

    std::cout << "Connected to server\n";
    return true;
}

void Client::Run()
{
    // Main menu loop for sending requests to the server.
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
            if (HandleDisconnectRequest())
            {
                break;
            }
        }
        else
        {
            std::cout << "Invalid choice\n";
        }
    }
}

bool Client::InitializeWinsock()
{
    // Load Winsock version 2.2.
    return WSAStartup(MAKEWORD(2, 2), &m_wsaData) == 0;
}

bool Client::CreateSocket()
{
    // Create a TCP socket.
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return m_socket != INVALID_SOCKET;
}

bool Client::SendAll(const char* buffer, int totalBytes)
{
    int bytesSent = 0;

    // Keep sending until the entire buffer is transmitted.
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

    // Keep receiving until the expected number of bytes is read.
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

    // Convert packet object into raw bytes before sending.
    if (!Shared::Serialization::SerializePacket(packet, buffer))
    {
        return false;
    }

    // Send the serialized packet bytes.
    if (!SendAll(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size())))
    {
        return false;
    }

    // Log outgoing packet.
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

    // Receive packet header first.
    if (!ReceiveAll(reinterpret_cast<char*>(headerBuffer.data()), static_cast<int>(headerBuffer.size())))
    {
        return false;
    }

    // Decode header fields.
    if (!Shared::Serialization::DeserializePacketHeader(headerBuffer, packet.header))
    {
        return false;
    }

    // Reject invalid or oversized payload sizes.
    if (!Shared::Serialization::ValidatePayloadSize(packet.header.payloadSize))
    {
        return false;
    }

    packet.payload.clear();

    // Receive payload only if packet contains one.
    if (packet.header.payloadSize > 0)
    {
        packet.payload.resize(packet.header.payloadSize);

        if (!ReceiveAll(reinterpret_cast<char*>(packet.payload.data()), static_cast<int>(packet.payload.size())))
        {
            return false;
        }
    }

    // Validate final packet structure.
    if (!packet.IsValid())
    {
        return false;
    }

    // Log incoming packet.
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

    // Add verification token as payload.
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

Shared::Packet Client::BuildDisconnectRequestPacket() const
{
    Shared::Packet packet;
    packet.header.type = Shared::PacketType::DISCONNECT_REQUEST;
    packet.header.timestamp = Shared::Serialization::GetCurrentTimestamp();
    packet.header.sensorCount = 0;
    packet.header.status = Shared::StatusCode::SUCCESS;
    packet.UpdatePayloadSize();

    return packet;
}

void Client::HandleVerifyRequest()
{
    // Build and send verification request to the server.
    Shared::Packet request = BuildVerifyRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send verification request\n";
        return;
    }

    // Receive acknowledgement packet first.
    Shared::Packet ackPacket;
    if (!ReceivePacket(ackPacket))
    {
        std::cout << "Failed to receive server acknowledgement\n";
        return;
    }

    // Handle verification rejection.
    if (ackPacket.header.type == Shared::PacketType::ERROR_PACKET)
    {
        std::string message = Shared::Serialization::ExtractTextPayload(ackPacket.payload);
        if (message.empty())
        {
            message = "Verification request rejected";
        }

        std::cout << message << "\n";
        return;
    }

    // Server must respond with ACK before sending actual response.
    if (ackPacket.header.type != Shared::PacketType::ACK)
    {
        std::cout << "Unexpected acknowledgement response\n";
        return;
    }

    // Receive final verification response.
    Shared::Packet response;
    if (!ReceivePacket(response))
    {
        std::cout << "Failed to receive verification response\n";
        return;
    }

    std::string message = Shared::Serialization::ExtractTextPayload(response.payload);
    if (message.empty())
    {
        message = "Verification complete";
    }

    std::cout << message << "\n";
}

void Client::HandleSensorRequest()
{
    // Build and send sensor data request.
    Shared::Packet request = BuildSensorRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send sensor request\n";
        return;
    }

    // Receive acknowledgement packet.
    Shared::Packet ackPacket;
    if (!ReceivePacket(ackPacket))
    {
        std::cout << "Failed to receive server acknowledgement\n";
        return;
    }

    // Handle request failure or missing verification.
    if (ackPacket.header.type == Shared::PacketType::ERROR_PACKET)
    {
        std::string message = Shared::Serialization::ExtractTextPayload(ackPacket.payload);

        if (!message.empty())
        {
            std::cout << message << "\n";
        }
        else if (ackPacket.header.status == Shared::StatusCode::NOT_VERIFIED)
        {
            std::cout << "Sensor request rejected: client not verified\n";
        }
        else
        {
            std::cout << "Sensor request failed\n";
        }

        return;
    }

    if (ackPacket.header.type != Shared::PacketType::ACK)
    {
        std::cout << "Unexpected acknowledgement response\n";
        return;
    }

    // Receive actual sensor response packet.
    Shared::Packet response;
    if (!ReceivePacket(response))
    {
        std::cout << "Failed to receive sensor response\n";
        return;
    }

    if (response.header.status != Shared::StatusCode::SUCCESS)
    {
        std::cout << "Sensor request failed\n";
        return;
    }

    // Decode sensor list from payload.
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
    // Build and send telemetry file request.
    Shared::Packet request = BuildTelemetryRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send telemetry request\n";
        return;
    }

    // Receive acknowledgement packet.
    Shared::Packet ackPacket;
    if (!ReceivePacket(ackPacket))
    {
        std::cout << "Failed to receive server acknowledgement\n";
        return;
    }

    // Handle telemetry request errors.
    if (ackPacket.header.type == Shared::PacketType::ERROR_PACKET)
    {
        std::string message = Shared::Serialization::ExtractTextPayload(ackPacket.payload);

        if (!message.empty())
        {
            std::cout << message << "\n";
        }
        else if (ackPacket.header.status == Shared::StatusCode::NOT_VERIFIED)
        {
            std::cout << "Telemetry request rejected: client not verified\n";
        }
        else
        {
            std::cout << "Telemetry request failed\n";
        }

        return;
    }

    if (ackPacket.header.type != Shared::PacketType::ACK)
    {
        std::cout << "Unexpected acknowledgement response\n";
        return;
    }

    // Receive telemetry file in chunks after ACK.
    if (!ReceiveTelemetryFile())
    {
        std::cout << "Telemetry transfer failed\n";
    }
}

bool Client::HandleDisconnectRequest()
{
    // Build and send disconnect request.
    Shared::Packet request = BuildDisconnectRequestPacket();

    if (!SendPacket(request))
    {
        std::cout << "Failed to send disconnect request\n";
        return false;
    }

    // Receive acknowledgement packet.
    Shared::Packet ackPacket;
    if (!ReceivePacket(ackPacket))
    {
        std::cout << "Failed to receive server acknowledgement\n";
        return false;
    }

    // Handle disconnect error response.
    if (ackPacket.header.type == Shared::PacketType::ERROR_PACKET)
    {
        std::string message = Shared::Serialization::ExtractTextPayload(ackPacket.payload);
        if (message.empty())
        {
            message = "Disconnect request failed";
        }

        std::cout << message << "\n";
        return false;
    }

    if (ackPacket.header.type != Shared::PacketType::ACK)
    {
        std::cout << "Unexpected acknowledgement response\n";
        return false;
    }

    // Receive final disconnect confirmation.
    Shared::Packet response;
    if (!ReceivePacket(response))
    {
        std::cout << "Failed to receive disconnect response\n";
        return false;
    }

    if (response.header.type != Shared::PacketType::DISCONNECT_RESPONSE)
    {
        std::cout << "Unexpected disconnect response\n";
        return false;
    }

    std::string message = Shared::Serialization::ExtractTextPayload(response.payload);
    if (message.empty())
    {
        message = "Disconnected successfully";
    }

    std::cout << message << "\n";

    // Log disconnect completion.
    Shared::Logger::LogEvent(
        Shared::CLIENT_LOG_FILE,
        "Client disconnect completed"
    );

    return true;
}

bool Client::ReceiveTelemetryFile()
{
    // Open output file where received telemetry data will be stored.
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

        // Receive next telemetry packet.
        if (!ReceivePacket(response))
        {
            outFile.close();
            return false;
        }

        // Handle server-side transfer errors.
        if (response.header.type == Shared::PacketType::ERROR_PACKET)
        {
            std::string message = Shared::Serialization::ExtractTextPayload(response.payload);

            if (!message.empty())
            {
                std::cout << message << "\n";
            }
            else if (response.header.status == Shared::StatusCode::NOT_VERIFIED)
            {
                std::cout << "Telemetry request rejected: client not verified\n";
            }
            else
            {
                std::cout << "Telemetry request failed\n";
            }

            outFile.close();
            return false;
        }

        // Write telemetry chunk data into file.
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
            // End of file transfer.
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
            // Reject unexpected packet types during file transfer.
            outFile.close();
            std::cout << "Unexpected packet received during telemetry transfer\n";
            return false;
        }
    }
}

void Client::DisplaySensors(const std::vector<Shared::SensorData>& sensors, std::uint64_t packetTimestamp) const
{
    // Display sensor values received from the server.
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

    // Show menu and read user input.
    std::cout << "\n1. Verify Connection\n2. Request Sensor Data\n3. Request Telemetry File\n4. Exit\nChoice: ";
    std::cin >> choice;

    // Handle invalid non-numeric input safely.
    if (std::cin.fail())
    {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        return -1;
    }

    return choice;
}
