#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "CppUnitTest.h"
#include "Packet.h"
#include "Serialization.h"
#include "PacketTypes.h"
#include "StatusCodes.h"
#include "Constants.h"
#include <winsock2.h>
#include <windows.h>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace IntegrationTests
{
    TEST_CLASS(IntegrationTestsClass)
    {
    public:

        // ================================
        // Helper: Start Server
        // ================================
        PROCESS_INFORMATION StartServer()
        {
           
            STARTUPINFO si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);

            CreateProcess(
                L"C:\\Users\\Jai\\Desktop\\Steve\\CSCN74000_Server\\x64\\Debug\\CSCN74000_Server.exe",
                NULL, NULL, NULL, FALSE,
                0, NULL, NULL,
                &si, &pi
            );

            Sleep(1000); // allow server to start
            return pi;
        }

        // ================================
        // Helper: Stop Server
        // ================================
        void StopServer(PROCESS_INFORMATION& pi)
        {
            // Ask process to exit nicely
            GenerateConsoleCtrlEvent(CTRL_CLOSE_EVENT, 0);

            WaitForSingleObject(pi.hProcess, 2000); // wait max 2 sec

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        // ================================
        // Helper: Connect Client
        // ================================
        SOCKET ConnectClient()
        {
            SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(54000);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            int result = connect(sock, (sockaddr*)&addr, sizeof(addr));

            Assert::AreNotEqual(result, SOCKET_ERROR);

            return sock;
        }

        bool SendPacketRaw(SOCKET sock, const Shared::Packet& packet)
        {
            std::vector<uint8_t> buffer;
            Shared::Serialization::SerializePacket(packet, buffer);

            return send(sock,
                reinterpret_cast<const char*>(buffer.data()),
                (int)buffer.size(), 0) > 0;
        }

        bool ReceivePacketRaw(SOCKET sock, Shared::Packet& packet)
        {
            std::vector<uint8_t> buffer(4096);

            int bytes = recv(sock,
                reinterpret_cast<char*>(buffer.data()),
                (int)buffer.size(), 0);

            if (bytes <= 0) return false;

            buffer.resize(bytes);
            return Shared::Serialization::DeserializePacket(buffer, packet);
        }

        // ======================================
        // TEST 1: Connection
        // ======================================
        TEST_METHOD(Test_Connection)
        {
            auto server = StartServer();

            SOCKET sock = ConnectClient();

            Assert::IsTrue(sock != INVALID_SOCKET);

            closesocket(sock);
            StopServer(server);
        }

        // ======================================
 // TEST 2: Verification Flow
 // ======================================
        TEST_METHOD(Test_Verification)
        {
            auto server = StartServer();
            SOCKET sock = ConnectClient();

            Shared::Packet pkt;
            pkt.header.type = Shared::PacketType::VERIFY_REQUEST;

            Shared::Serialization::BuildTextPayload(
                Shared::EXPECTED_VERIFICATION_TOKEN,
                pkt.payload
            );

            pkt.UpdatePayloadSize();

            SendPacketRaw(sock, pkt);

            Shared::Packet response;
            bool received = ReceivePacketRaw(sock, response);

            Assert::IsTrue(received);

            // Current behavior to  ACK
            Assert::AreEqual(
                (int)Shared::PacketType::ACK,
                (int)response.header.type
            );

            closesocket(sock);
            StopServer(server);
        }
        // ======================================
 // TEST 3: Sensor Request WITHOUT Verification
 // ======================================
        TEST_METHOD(Test_SensorWithoutVerification)
        {
            auto server = StartServer();
            SOCKET sock = ConnectClient();

            Shared::Packet pkt;
            pkt.header.type = Shared::PacketType::SENSOR_REQUEST;
            pkt.UpdatePayloadSize();

            SendPacketRaw(sock, pkt);

            Shared::Packet response;
            bool received = ReceivePacketRaw(sock, response);

            Assert::IsTrue(received);

            Assert::AreEqual(
                (int)Shared::PacketType::ERROR_PACKET,
                (int)response.header.type
            );

            closesocket(sock);
            StopServer(server);
        }
    };
}