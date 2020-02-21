
#include <stdio.h>
#include <string.h>
#include "TcpServer.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>


void my_handlerSIGINT(int s)
{
    throw s;
}

int main()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handlerSIGINT;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    signal(SIGPIPE, SIG_IGN);


   TCPServer server;
    try
    {
        server.initialiserSocket();
        server.bindServer();
        server.listenServer();
        server.AcceptClients();
    }
    catch (int &i)
    {
       server.closeServer();
    }
    return 0;
}
