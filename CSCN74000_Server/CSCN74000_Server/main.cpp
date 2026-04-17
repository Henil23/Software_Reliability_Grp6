#include <iostream>
#include "Server.h"
// server startup application
int main()
{
    Server server;

    if (!server.Start())
    {
        std::cout << "Server startup failed.\n";
        return 1;
    }

    server.Run();
    server.Stop();

    return 0;
}
