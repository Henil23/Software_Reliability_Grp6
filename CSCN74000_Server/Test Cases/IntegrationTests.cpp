#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "CppUnitTest.h"

#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

TEST_METHOD(Test_Connection)
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

    Sleep(1000);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int result = connect(sock, (sockaddr*)&addr, sizeof(addr));

    Assert::AreNotEqual(result, SOCKET_ERROR);

    closesocket(sock);

    TerminateProcess(pi.hProcess, 0);
}