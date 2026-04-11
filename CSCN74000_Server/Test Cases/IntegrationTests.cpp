#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "CppUnitTest.h"

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
            TerminateProcess(pi.hProcess, 0);
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
        // TEST 2: Basic Send/Receive
        // ======================================
        TEST_METHOD(Test_Send_Receive)
        {
            auto server = StartServer();

            SOCKET sock = ConnectClient();

            const char* msg = "HELLO";
            send(sock, msg, 5, 0);

            char buffer[1024];
            int received = recv(sock, buffer, sizeof(buffer), 0);

            Assert::IsTrue(received >= 0);

            closesocket(sock);
            StopServer(server);
        }


        // ======================================
        // TEST 3: Invalid Request Handling
        // ======================================
        TEST_METHOD(Test_Invalid_Request)
        {
            auto server = StartServer();

            SOCKET sock = ConnectClient();

            const char* msg = "INVALID_DATA";
            send(sock, msg, 12, 0);

            char buffer[1024];
            int received = recv(sock, buffer, sizeof(buffer), 0);

            Assert::IsTrue(received >= 0);

            closesocket(sock);
            StopServer(server);
        }


        // ======================================
        // TEST 4: Multiple Requests
        // ======================================
        TEST_METHOD(Test_Multiple_Requests)
        {
            auto server = StartServer();

            SOCKET sock = ConnectClient();

            const char* msg = "PING";

            for (int i = 0; i < 3; i++)
            {
                send(sock, msg, 4, 0);

                char buffer[1024];
                int received = recv(sock, buffer, sizeof(buffer), 0);

                Assert::IsTrue(received >= 0);
            }

            closesocket(sock);
            StopServer(server);
        }
    };
}