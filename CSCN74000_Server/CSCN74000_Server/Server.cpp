#include "Server.h"

#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "Constants.h"
#include "Serialization.h"
#include "PacketUtils.h"
#include "Logger.h"

#pragma comment(lib, "Ws2_32.lib")

namespace
{
    double RandomInRange(double minValue, double maxValue)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist(minValue, maxValue);
        return dist(gen);
    }

    std::vector<Shared::SensorData> GenerateCurrentSensors()
    {
        std::vector<Shared::SensorData> sensors;
        const std::uint64_t timestamp = Shared::Serialization::GetCurrentTimestamp();

        const double altitude = RandomInRange(33000.0, 37000.0);
        const double speed = RandomInRange(420.0, 480.0);
        const double temperature = RandomInRange(-58.0, -42.0);
        const double fuelLevel = RandomInRange(35.0, 95.0);
        const double engineRpm = RandomInRange(7800.0, 9100.0);
        const double oilPressure = RandomInRange(45.0, 70.0);
        const double cabinPressure = RandomInRange(10.2, 11.4);
        const double heading = RandomInRange(0.0, 359.9);
        const double pitch = RandomInRange(-6.0, 6.0);
        const double roll = RandomInRange(-12.0, 12.0);
        const double verticalSpeed = RandomInRange(-1800.0, 1800.0);

        sensors.emplace_back("Altitude", altitude, "ft", timestamp);
        sensors.emplace_back("Speed", speed, "knots", timestamp);
        sensors.emplace_back("Temperature", temperature, "C", timestamp);
        sensors.emplace_back("Fuel Level", fuelLevel, "%", timestamp);
        sensors.emplace_back("Engine RPM", engineRpm, "rpm", timestamp);
        sensors.emplace_back("Oil Pressure", oilPressure, "psi", timestamp);
        sensors.emplace_back("Cabin Pressure", cabinPressure, "psi", timestamp);
        sensors.emplace_back("Heading", heading, "deg", timestamp);
        sensors.emplace_back("Pitch", pitch, "deg", timestamp);
        sensors.emplace_back("Roll", roll, "deg", timestamp);
        sensors.emplace_back("Vertical Speed", verticalSpeed, "fpm", timestamp);

        return sensors;
    }
}

Server::Server()
    : m_listenSocket(INVALID_SOCKET),
    m_clientSocket(INVALID_SOCKET),
    m_state(Shared::ServerState::WAITING_FOR_CONNECTION),
    m_isRunning(false)
{
    ZeroMemory(&m_wsaData, sizeof(m_wsaData));
    ZeroMemory(&m_serverAddress, sizeof(m_serverAddress));
}

Server::~Server()
{
    Stop();
}

bool Server::Start()
{
    if (!InitializeWinsock())
    {
        std::cout << "Winsock initialization failed.\n";
        return false;
    }

    if (!CreateListenSocket())
    {
        std::cout << "Listen socket creation failed.\n";
        CleanupWinsock();
        return false;
    }

    if (!BindSocket())
    {
        std::cout << "Bind failed.\n";
        Stop();
        return false;
    }

    if (!ListenForClient())
    {
        std::cout << "Listen failed.\n";
        Stop();
        return false;
    }

    m_isRunning = true;
    SetState(Shared::ServerState::WAITING_FOR_CONNECTION);

    Shared::Logger::LogEvent(
        Shared::SERVER_LOG_FILE,
        "Server started"
    );

    std::cout << "Server started on port " << Shared::DEFAULT_SERVER_PORT << ".\n";
    return true;
}

void Server::Stop()
{
    m_isRunning = false;
    CloseClientSocket();
    CloseListenSocket();
    CleanupWinsock();
}

void Server::Run()
{
    if (!m_isRunning)
    {
        std::cout << "Server is not running.\n";
        return;
    }

    while (m_isRunning)
    {
        if (!AcceptClient())
        {
            std::cout << "Client accept failed.\n";
            continue;
        }

        SetState(Shared::ServerState::CONNECTED);

        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Client connected"
        );

        std::cout << "Client connected.\n";

        while (m_isRunning)
        {
            Shared::Packet packet;

            if (!ReceivePacket(packet))
            {
                Shared::Logger::LogEvent(
                    Shared::SERVER_LOG_FILE,
                    "Client disconnected or packet receive failed"
                );

                std::cout << "Client disconnected or packet receive failed.\n";
                break;
            }

            std::cout << "Received packet. Type=" << static_cast<std::uint32_t>(packet.header.type)
                << ", PayloadSize=" << packet.header.payloadSize
                << ", State=" << GetStateString() << "\n";

            if (packet.header.type == Shared::PacketType::TELEMETRY_REQUEST)
            {
                if (!HandleTelemetryRequest())
                {
                    Shared::Logger::LogEvent(
                        Shared::SERVER_LOG_FILE,
                        "Telemetry transfer failed"
                    );

                    std::cout << "Telemetry transfer failed.\n";
                    break;
                }

                continue;
            }

            Shared::Packet response;

            switch (packet.header.type)
            {
            case Shared::PacketType::VERIFY_REQUEST:
                response = HandleVerification(packet);
                break;

            case Shared::PacketType::SENSOR_REQUEST:
                response = HandleSensorRequest();
                break;

            default:
                Shared::Logger::LogEvent(
                    Shared::SERVER_LOG_FILE,
                    "Invalid command received"
                );

                response = Shared::PacketUtils::CreateSimplePacket(
                    Shared::PacketType::ERROR_PACKET,
                    Shared::StatusCode::INVALID_COMMAND
                );
                break;
            }

            if (!SendPacket(response))
            {
                Shared::Logger::LogEvent(
                    Shared::SERVER_LOG_FILE,
                    "Failed to send response packet"
                );

                std::cout << "Failed to send response packet.\n";
                break;
            }
        }

        CloseClientSocket();
        SetState(Shared::ServerState::WAITING_FOR_CONNECTION);

        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Waiting for next client connection"
        );
    }
}

bool Server::InitializeWinsock()
{
    return WSAStartup(MAKEWORD(2, 2), &m_wsaData) == 0;
}

bool Server::CreateListenSocket()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return m_listenSocket != INVALID_SOCKET;
}

bool Server::BindSocket()
{
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_addr.s_addr = INADDR_ANY;
    m_serverAddress.sin_port = htons(Shared::DEFAULT_SERVER_PORT);

    const int result = bind(
        m_listenSocket,
        reinterpret_cast<sockaddr*>(&m_serverAddress),
        sizeof(m_serverAddress));

    return result != SOCKET_ERROR;
}

bool Server::ListenForClient()
{
    return listen(m_listenSocket, SOMAXCONN) != SOCKET_ERROR;
}

bool Server::AcceptClient()
{
    std::cout << "Waiting for client connection...\n";
    m_clientSocket = accept(m_listenSocket, nullptr, nullptr);
    return m_clientSocket != INVALID_SOCKET;
}

bool Server::ReceiveAll(char* buffer, int totalBytes)
{
    int bytesReceived = 0;

    while (bytesReceived < totalBytes)
    {
        const int result = recv(
            m_clientSocket,
            buffer + bytesReceived,
            totalBytes - bytesReceived,
            0);

        if (result <= 0)
        {
            return false;
        }

        bytesReceived += result;
    }

    return true;
}

bool Server::SendAll(const char* buffer, int totalBytes)
{
    int bytesSent = 0;

    while (bytesSent < totalBytes)
    {
        const int result = send(
            m_clientSocket,
            buffer + bytesSent,
            totalBytes - bytesSent,
            0);

        if (result == SOCKET_ERROR)
        {
            return false;
        }

        bytesSent += result;
    }

    return true;
}

bool Server::ReceivePacket(Shared::Packet& outPacket)
{
    const std::size_t headerSize = Shared::Serialization::GetPacketHeaderSize();
    std::vector<std::uint8_t> headerBuffer(headerSize);

    if (!ReceiveAll(reinterpret_cast<char*>(headerBuffer.data()), static_cast<int>(headerBuffer.size())))
    {
        return false;
    }

    if (!Shared::Serialization::DeserializePacketHeader(headerBuffer, outPacket.header))
    {
        return false;
    }

    if (!Shared::Serialization::ValidatePayloadSize(outPacket.header.payloadSize))
    {
        return false;
    }

    outPacket.payload.clear();

    if (outPacket.header.payloadSize > 0)
    {
        outPacket.payload.resize(outPacket.header.payloadSize);

        if (!ReceiveAll(reinterpret_cast<char*>(outPacket.payload.data()), static_cast<int>(outPacket.payload.size())))
        {
            return false;
        }
    }

    if (!outPacket.IsValid())
    {
        return false;
    }

    Shared::Logger::LogPacket(
        Shared::SERVER_LOG_FILE,
        outPacket,
        Shared::LogDirection::RECEIVED
    );

    return true;
}

bool Server::SendPacket(const Shared::Packet& packet)
{
    std::vector<std::uint8_t> serializedPacket;

    if (!Shared::Serialization::SerializePacket(packet, serializedPacket))
    {
        return false;
    }

    if (!SendAll(reinterpret_cast<const char*>(serializedPacket.data()), static_cast<int>(serializedPacket.size())))
    {
        return false;
    }

    Shared::Logger::LogPacket(
        Shared::SERVER_LOG_FILE,
        packet,
        Shared::LogDirection::SENT
    );

    return true;
}

Shared::Packet Server::HandleVerification(const Shared::Packet& packet)
{
    std::string token = Shared::Serialization::ExtractTextPayload(packet.payload);

    if (token == Shared::EXPECTED_VERIFICATION_TOKEN)
    {
        SetState(Shared::ServerState::VERIFIED);

        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Verification successful"
        );

        return Shared::PacketUtils::CreateTextPacket(
            Shared::PacketType::VERIFY_RESPONSE,
            "Verification Successful",
            Shared::StatusCode::SUCCESS
        );
    }

    Shared::Logger::LogEvent(
        Shared::SERVER_LOG_FILE,
        "Verification failed"
    );

    return Shared::PacketUtils::CreateTextPacket(
        Shared::PacketType::VERIFY_RESPONSE,
        "Verification Failed",
        Shared::StatusCode::AUTH_FAILED
    );
}

Shared::Packet Server::HandleSensorRequest()
{
    if (!IsVerified())
    {
        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Sensor request rejected: not verified"
        );

        return Shared::PacketUtils::CreateSimplePacket(
            Shared::PacketType::ERROR_PACKET,
            Shared::StatusCode::NOT_VERIFIED
        );
    }

    SetState(Shared::ServerState::SENSOR_DATA);

    const std::vector<Shared::SensorData> sensors = GenerateCurrentSensors();

    Shared::Logger::LogEvent(
        Shared::SERVER_LOG_FILE,
        "Randomized sensor response generated"
    );

    return Shared::PacketUtils::CreateSensorResponsePacket(sensors);
}

bool Server::HandleTelemetryRequest()
{
    if (!IsVerified())
    {
        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Telemetry request rejected: not verified"
        );

        Shared::Packet errorPacket = Shared::PacketUtils::CreateSimplePacket(
            Shared::PacketType::ERROR_PACKET,
            Shared::StatusCode::NOT_VERIFIED
        );

        return SendPacket(errorPacket);
    }

    SetState(Shared::ServerState::TELEMETRY_TRANSFER);

    Shared::Logger::LogEvent(
        Shared::SERVER_LOG_FILE,
        "Telemetry transfer started"
    );

    bool success = SendTelemetryFile();

    SetState(Shared::ServerState::VERIFIED);

    if (success)
    {
        Shared::Logger::LogEvent(
            Shared::SERVER_LOG_FILE,
            "Telemetry transfer completed"
        );
    }

    return success;
}

bool Server::SendTelemetryFile()
{
    std::ifstream file(Shared::TELEMETRY_FILE_PATH, std::ios::binary);
    if (!file.is_open())
    {
        Shared::Packet errorPacket = Shared::PacketUtils::CreateTextPacket(
            Shared::PacketType::ERROR_PACKET,
            "Telemetry file open failed",
            Shared::StatusCode::INTERNAL_ERROR
        );

        return SendPacket(errorPacket);
    }

    std::vector<char> chunkBuffer(Shared::TELEMETRY_CHUNK_SIZE);

    while (file.good())
    {
        file.read(chunkBuffer.data(), static_cast<std::streamsize>(chunkBuffer.size()));
        std::streamsize bytesRead = file.gcount();

        if (bytesRead <= 0)
        {
            break;
        }

        Shared::Packet chunkPacket;
        chunkPacket.header.type = Shared::PacketType::TELEMETRY_CHUNK;
        chunkPacket.header.timestamp = Shared::Serialization::GetCurrentTimestamp();
        chunkPacket.header.sensorCount = 0;
        chunkPacket.header.status = Shared::StatusCode::SUCCESS;

        chunkPacket.payload.assign(
            chunkBuffer.begin(),
            chunkBuffer.begin() + bytesRead
        );

        chunkPacket.UpdatePayloadSize();

        if (!SendPacket(chunkPacket))
        {
            return false;
        }
    }

    Shared::Packet completePacket = Shared::PacketUtils::CreateTextPacket(
        Shared::PacketType::TELEMETRY_COMPLETE,
        "Telemetry transfer complete",
        Shared::StatusCode::SUCCESS
    );

    return SendPacket(completePacket);
}

bool Server::IsVerified() const
{
    return m_state == Shared::ServerState::VERIFIED ||
        m_state == Shared::ServerState::SENSOR_DATA;
}

void Server::CloseClientSocket()
{
    if (m_clientSocket != INVALID_SOCKET)
    {
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
}

void Server::CloseListenSocket()
{
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
}

void Server::CleanupWinsock()
{
    WSACleanup();
}

void Server::SetState(Shared::ServerState newState)
{
    m_state = newState;
}

std::string Server::GetStateString() const
{
    switch (m_state)
    {
    case Shared::ServerState::WAITING_FOR_CONNECTION:
        return "WAITING_FOR_CONNECTION";
    case Shared::ServerState::CONNECTED:
        return "CONNECTED";
    case Shared::ServerState::VERIFIED:
        return "VERIFIED";
    case Shared::ServerState::SENSOR_DATA:
        return "SENSOR_DATA";
    case Shared::ServerState::TELEMETRY_TRANSFER:
        return "TELEMETRY_TRANSFER";
    case Shared::ServerState::DISCONNECTING:
        return "DISCONNECTING";
    case Shared::ServerState::ERROR_STATE:
        return "ERROR_STATE";
    default:
        return "UNKNOWN";
    }
}