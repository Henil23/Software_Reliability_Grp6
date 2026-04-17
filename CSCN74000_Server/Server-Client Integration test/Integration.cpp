#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "gtest/gtest.h"
#include "pch.h"
#include "Server.h"
#include "Packet.h"
#include "Serialization.h"
#include "PacketUtils.h"
#include "Constants.h"

#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace Shared;

class IntegrationTest : public ::testing::Test
{
protected:
    Server* server = nullptr;   // FIXED
    std::thread serverThread;

    void SetUp() override
    {
        server = new Server(false); // REAL MODE

        serverThread = std::thread([&]() {
            server->Start();
            server->Run();
            });

        Sleep(1000); // allow server to start
    }

    void TearDown() override
    {
        server->Stop();

        if (serverThread.joinable())
            serverThread.detach();   // safe for now

        delete server;
    }
};



// ======================================
// TEST 1: Verify Connection
// ======================================
TEST_F(IntegrationTest, VerifyConnection)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_NE(connect(sock, (sockaddr*)&addr, sizeof(addr)), SOCKET_ERROR);

    Packet pkt;
    pkt.header.type = PacketType::VERIFY_REQUEST;
    Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, pkt.payload);
    pkt.UpdatePayloadSize();

    std::vector<uint8_t> buffer;
    Serialization::SerializePacket(pkt, buffer);

    send(sock, (char*)buffer.data(), (int)buffer.size(), 0);

    char recvBuf[1024];
    int received = recv(sock, recvBuf, sizeof(recvBuf), 0);

    ASSERT_GT(received, 0);

    closesocket(sock);
}



// ======================================
// TEST 2: Sensor After Verification
// ======================================
TEST_F(IntegrationTest, SensorAfterVerification)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    // VERIFY
    Packet verifyPkt;
    verifyPkt.header.type = PacketType::VERIFY_REQUEST;
    Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, verifyPkt.payload);
    verifyPkt.UpdatePayloadSize();

    std::vector<uint8_t> buffer;
    Serialization::SerializePacket(verifyPkt, buffer);
    send(sock, (char*)buffer.data(), (int)buffer.size(), 0);

    char recvBuf[1024];
    recv(sock, recvBuf, sizeof(recvBuf), 0);

    // SENSOR REQUEST
    Packet sensorPkt;
    sensorPkt.header.type = PacketType::SENSOR_REQUEST;
    sensorPkt.UpdatePayloadSize();

    buffer.clear();
    Serialization::SerializePacket(sensorPkt, buffer);
    send(sock, (char*)buffer.data(), (int)buffer.size(), 0);

    int received = recv(sock, recvBuf, sizeof(recvBuf), 0);

    ASSERT_GT(received, 0);

    closesocket(sock);
}



// ======================================
// TEST 3: Sensor Without Verification
// ======================================
TEST_F(IntegrationTest, SensorWithoutVerification)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    Packet pkt;
    pkt.header.type = PacketType::SENSOR_REQUEST;
    pkt.UpdatePayloadSize();

    std::vector<uint8_t> buffer;
    Serialization::SerializePacket(pkt, buffer);

    send(sock, (char*)buffer.data(), (int)buffer.size(), 0);

    char recvBuf[1024];
    int received = recv(sock, recvBuf, sizeof(recvBuf), 0);

    ASSERT_GT(received, 0);

    closesocket(sock);
}



// ======================================
// TEST 4: Disconnect
// ======================================
TEST_F(IntegrationTest, DisconnectFlow)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (sockaddr*)&addr, sizeof(addr));

    Packet pkt;
    pkt.header.type = PacketType::DISCONNECT_REQUEST;
    pkt.UpdatePayloadSize();

    std::vector<uint8_t> buffer;
    Serialization::SerializePacket(pkt, buffer);

    send(sock, (char*)buffer.data(), (int)buffer.size(), 0);

    char recvBuf[1024];
    int received = recv(sock, recvBuf, sizeof(recvBuf), 0);

    ASSERT_GT(received, 0);

    closesocket(sock);
}