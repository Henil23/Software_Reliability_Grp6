#include <iostream>
#include "Client.h"
// client startup 
int main()
{
    Client client;

    if (!client.Start())
    {
        std::cout << "Client startup failed.\n";
        return 1;
    }

    if (!client.ConnectToServer())
    {
        std::cout << "Client connection failed.\n";
        client.Stop();
        return 1;
    }

    client.Run();
    client.Stop();

    return 0;
}
