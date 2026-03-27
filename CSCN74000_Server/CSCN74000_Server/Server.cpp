#include "Server.h"

#include <iostream>

#include "Packet.h"
#include "ServerState.h"
#include "Constants.h"
#include "Serialization.h"
#include "PacketUtils.h"

#pragma comment(lib, "Ws2_32.lib")

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

    if (!AcceptClient())
    {
        std::cout << "Client accept failed.\n";
        return;
    }

    SetState(Shared::ServerState::CONNECTED);
    std::cout << "Client connected.\n";

    while (m_isRunning)
    {
        Shared::Packet packet;

        if (!ReceivePacket(packet))
        {
            std::cout << "Client disconnected or packet receive failed.\n";
            break;
        }

        std::cout << "Received packet. Type=" << static_cast<std::uint32_t>(packet.header.type)
            << ", PayloadSize=" << packet.header.payloadSize
            << ", State=" << GetStateString() << "\n";

        Shared::Packet response;

        switch (packet.header.type)
        {
        case Shared::PacketType::VERIFY_REQUEST:
            response = HandleVerification(packet);
            break;

        case Shared::PacketType::SENSOR_REQUEST:
            response = HandleSensorRequest(packet);
            break;

        default:
            response = Shared::PacketUtils::CreateSimplePacket(
                Shared::PacketType::ERROR_PACKET,
                Shared::StatusCode::INVALID_COMMAND
            );
            break;
        }

        if (!SendPacket(response))
        {
            std::cout << "Failed to send response packet.\n";
            break;
        }
    }

    CloseClientSocket();
    SetState(Shared::ServerState::WAITING_FOR_CONNECTION);
}   

bool Server::InitializeWinsock()
{
    const int result = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
    return result == 0;
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
    const int result = listen(m_listenSocket, SOMAXCONN);
    return result != SOCKET_ERROR;
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

    return outPacket.IsValid();
}

bool Server::SendPacket(const Shared::Packet& packet)
{
    std::vector<std::uint8_t> serializedPacket;

    if (!Shared::Serialization::SerializePacket(packet, serializedPacket))
    {
        return false;
    }

    return SendAll(reinterpret_cast<const char*>(serializedPacket.data()), static_cast<int>(serializedPacket.size()));
}

bool Server::IsVerified() const
{
    return m_state == Shared::ServerState::VERIFIED;
}

Shared::Packet Server::HandleVerification(const Shared::Packet& packet)
{
    std::string token = Shared::Serialization::ExtractTextPayload(packet.payload);

    if (token == Shared::EXPECTED_VERIFICATION_TOKEN)
    {
        SetState(Shared::ServerState::VERIFIED);

        std::cout << "State changed to VERIFIED\n";

        return Shared::PacketUtils::CreateTextPacket(
            Shared::PacketType::VERIFY_RESPONSE,
            "Verification Successful",
            Shared::StatusCode::SUCCESS
        );
    }

    return Shared::PacketUtils::CreateTextPacket(
        Shared::PacketType::VERIFY_RESPONSE,
        "Verification Failed",
        Shared::StatusCode::AUTH_FAILED
    );
}

Shared::Packet Server::HandleSensorRequest(const Shared::Packet& packet)
{
    if (!IsVerified())
    {
        return Shared::PacketUtils::CreateSimplePacket(
            Shared::PacketType::ERROR_PACKET,
            Shared::StatusCode::NOT_VERIFIED
        );
    }

    SetState(Shared::ServerState::SENSOR_DATA);

    std::vector<Shared::SensorData> sensors;

    auto timestamp = Shared::Serialization::GetCurrentTimestamp();

    sensors.emplace_back("Altitude", 35000, "ft", timestamp);
    sensors.emplace_back("Speed", 450, "knots", timestamp);
    sensors.emplace_back("Temperature", -50, "C", timestamp);

    return Shared::PacketUtils::CreateSensorResponsePacket(sensors);
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