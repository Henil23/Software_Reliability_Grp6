#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Packet.h"
#include "PacketUtils.h"
#include "Serialization.h"
#include "Constants.h"

#pragma comment(lib, "Ws2_32.lib")

void SendPacket(SOCKET sock, const Shared::Packet& packet)
{
    std::vector<uint8_t> buffer;
    Shared::Serialization::SerializePacket(packet, buffer);

    send(sock, (char*)buffer.data(), buffer.size(), 0);
}

bool ReceivePacket(SOCKET sock, Shared::Packet& packet)
{
    size_t headerSize = Shared::Serialization::GetPacketHeaderSize();
    std::vector<uint8_t> headerBuffer(headerSize);

    int received = recv(sock, (char*)headerBuffer.data(), headerSize, 0);
    if (received <= 0) return false;

    Shared::Serialization::DeserializePacketHeader(headerBuffer, packet.header);

    if (packet.header.payloadSize > 0)
    {
        packet.payload.resize(packet.header.payloadSize);
        recv(sock, (char*)packet.payload.data(), packet.payload.size(), 0);
    }

    return true;
}

void DisplaySensors(const std::vector<Shared::SensorData>& sensors)
{
    std::cout << "\n--- Sensor Data ---\n";

    for (const auto& s : sensors)
    {
        std::cout << s.name << " : " << s.value << " " << s.unit << "\n";
    }
}

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(Shared::DEFAULT_SERVER_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0)
    {
        std::cout << "Connection failed\n";
        return 1;
    }

    std::cout << "Connected to server\n";

    int choice;

    while (true)
    {
        std::cout << "\n1. Verify Connection\n2. Request Sensor Data\n3. Exit\nChoice: ";
        std::cin >> choice;

        if (choice == 1)
        {
            Shared::Packet packet;
            packet.header.type = Shared::PacketType::VERIFY_REQUEST;
            packet.header.timestamp = Shared::Serialization::GetCurrentTimestamp();

            Shared::Serialization::BuildTextPayload(
                Shared::EXPECTED_VERIFICATION_TOKEN,
                packet.payload
            );

            packet.UpdatePayloadSize();

            SendPacket(sock, packet);

            Shared::Packet response;
            ReceivePacket(sock, response);

            std::string msg = Shared::Serialization::ExtractTextPayload(response.payload);

            std::cout << "Server Response: " << msg << "\n";
        }

        else if (choice == 2)
        {
            Shared::Packet packet = Shared::PacketUtils::CreateSimplePacket(
                Shared::PacketType::SENSOR_REQUEST
            );

            SendPacket(sock, packet);

            Shared::Packet response;
            ReceivePacket(sock, response);

            if (response.header.status != Shared::StatusCode::SUCCESS)
            {
                std::cout << "Error: ";

                if (response.header.status == Shared::StatusCode::NOT_VERIFIED)
                    std::cout << "Not Verified\n";
                else
                    std::cout << "Request Failed\n";

                continue;  
            }

            std::vector<Shared::SensorData> sensors;
            Shared::Serialization::DeserializeSensorDataList(response.payload, sensors);

            DisplaySensors(sensors);
        }

        else if (choice == 3)
        {
            break;
        }
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}