#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <map>
#include <netinet/in.h> // sockaddr_in, IPPROTO_TCP
#include <mutex>



const int INVALID_SOCKET = -1; //valeur pour socket invalide
const int BACKLOG_LIST = 8; //la taille de la file de connexions en attente


struct Client
{
    int sckt = {-1} ;
    sockaddr_in addr = {0};

};


class TCPServer
{
public:
    TCPServer();
    ~TCPServer();


    //méthode public de la classe TCP server

    void initialiserSocket();
    void bindServer();
    void listenServer();
    void AcceptClients();
    void closeServer();

    static char* generateUniqueID();


private:
    int _serverSocket;
    sockaddr_in _addr;
    unsigned short _port;

    std::mutex _mutexListeClient;
    std::map<int , Client>_listeClients;

    void deconnectClient( Client);
    void sendNumberToClient();


};

#endif // TCPSERVER_H
