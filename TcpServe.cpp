#include "TcpServer.h"
#include "sys/socket.h"
#include <iostream>
#include <string>
#include <thread>
#include <stdio.h>
#include<stdlib.h>
#include <unistd.h>  // close

#include <string.h>


TCPServer::TCPServer()
{
    _serverSocket = -1;

}

TCPServer::~TCPServer()
{
    closeServer();
}


///
/// \brief TCPServer::generateUniqueID
/// static function for unique key generating based on the current time in second
/// \return char*
///
char* TCPServer::generateUniqueID()
{
    //thread generate and send  a server wide unique ID
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    int timeSec ;
    timeSec = (now->tm_hour) * 36000  + (now->tm_min) * 60  +  now->tm_sec;
    char *str = (char*)  malloc(8); //taille max sec Id '8' '6' '4' '0' '0'  + '\n' + '\0'
    memset(str, '\0', 8);
    sprintf(str, "%d \n", timeSec);
    return str;

}

///
/// \brief TCPServer::initialiserSocket
/// cette renvoie le descripteur de votre socket nouvellement créé
/// et on l'écrit dans l'attribut _serverSocket
///
void TCPServer::initialiserSocket()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_serverSocket == INVALID_SOCKET)
    {
        std::cout << "Erreur initialisation socket : " << errno ;
        throw -2;
    }

    std::cout << "Port ? ";
    std::cin >> _port;

    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(_port);//convert network to host format
    _addr.sin_family = AF_INET;
}

///
/// \brief TCPServer::bindServer
/// bind lie un socket avec une structure sockaddr _addr
///
void TCPServer::bindServer()
{
    int res = bind(_serverSocket, (sockaddr*)&_addr, sizeof(_addr));
    if (res != 0)
    {
        std::cout << "Erreur bind : " << errno ;
        throw -3;
    }
}

///
/// \brief TCPServer::listenServer
/// Cette fonction définie la taille de la file de connexions en attente pour votre socket _serverSocket.
///
void TCPServer::listenServer()
{
    int res = listen(_serverSocket, BACKLOG_LIST);
    if (res != 0)
    {
        std::cout << "Erreur listen : " << errno;
        throw -4 ;
    }
    else
    {
        std::cout << "Server demarre sur le port " << _port << std::endl;
    }
}


///
/// \brief TCPServer::AcceptClients
/// Cette fonction accepte la connexion d'un socket sur le socket sock. Le socket aura été préalablement lié avec un port avec la fonction bind
//L'argument adresse sera remplie avec les informations du client qui s'est connecté.
//Cette fonction retourne un nouveau socket, qui devra être utilisé pour communiquer avec le client.
///
void TCPServer::AcceptClients()
{

    //ce thread surveille les clients et envoie une réponse avec le nombre de client
    // connectés suite à la réception d'un '/n'

    std::thread([this]() {
        this->sendNumberToClient();

    }).detach();


    //send life signe to all connected client the unique ID key
    for (;;)
    {
        Client client;

        socklen_t addrlen = sizeof(client.addr);
        if(_listeClients.size() < 8)
        {

            client.sckt = accept(_serverSocket, (sockaddr*)(&client.addr), &addrlen);
            if (client.sckt != INVALID_SOCKET)
            {
                //ajout du nouveau client dans la liste
                _listeClients[client.sckt] = client;
                //démaragge du thread client , on passe le this pour acceder à des attribut membres
                //telque size de _listeClients
                std::thread([client ,this]() {
                    std::cout << "client connected :" << client.sckt << std::endl;

                    for(;;)
                    {
                        char* str;
                        str = TCPServer::generateUniqueID();
                        int ret = send(client.sckt,str , 8, 0);
                        free(str);
                        if (ret == 0 || ret == -1)
                        {
                            break;
                        }
                        sleep(1);

                    }
                    std::cout << "Client disconnected [" << client.sckt << "]" << std::endl;

                    //section critique protection de la liste client
                    _mutexListeClient.lock();
                    _listeClients.erase(client.sckt);
                    _mutexListeClient.unlock();
                }).detach();
            }
            else
                break;
        }
    }
}


void TCPServer::closeServer()
{
    //fermer les connexions client sauvgarder dans le tableau clients
    std::cout << "\n Server is shut down and clients are disconnected " <<std::endl;


    //envoyer Bye à tous les clients et les déconnecter

    for (std::map<int,Client>::iterator it=_listeClients.begin(); it!=_listeClients.end(); ++it)
    {
        //construire le message de Bye pour envoyer aux clients

        Client client;
        client = it->second;

        char str[6] ;
        sprintf(str, "%s", "Bye/n");

        send(client.sckt, str, sizeof(str), 0);
        std::cout << "coupure de liason avec le client [ " <<client.sckt << " ]"<< std::endl;
        //close socket client
        close(client.sckt);
        client.sckt = -1;
    }

    //clean la liste des Clients enregistrés
    _listeClients.clear();

    //Fermer le socket server
    close(_serverSocket);
    _serverSocket = -1;

    //quitter proprement l'application avec un exit
    exit(1);

}

///
/// \brief TCPServer::sendNumberToClient
/// cette fonction vérifié la réception d'un message par le serveur
/// si le message reçu est \n , elle envoie le nombre du client connecté à cet instant
///
void TCPServer::sendNumberToClient()
{
    int maxSock = 0;
    for(;;)
    {
        _mutexListeClient.lock();
        if(_listeClients.size() > 0)
        {
            maxSock = _listeClients.rbegin()->first;
        }

        fd_set readfs;
        int ret = 0;
        FD_ZERO(&readfs);
        FD_SET(maxSock, &readfs);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100;

        //on utilise la fonction select afin de vérifier la réception d'un message
        //depuis les sockets client

        ret = select(maxSock + 1, &readfs, NULL, NULL, &tv);
        if (ret < 0)
        {
            //erreur de la select
            throw -5;

        }

        for (std::map<int,Client>::iterator it=_listeClients.begin(); it!=_listeClients.end(); ++it)
        {

            if (FD_ISSET(it->first, &readfs))
            {
                char buffer[200] = { 0 };
                int ret = recv(it->first, buffer, 199, 0);
                if (ret == 0 || ret == -1)
                {
                    break;
                }
                char key[] = "\n\0";
                //comparaison du message reçu avec la chaine "\n\0"
                if(strcmp(key, buffer) == 0)
                {
                    char *str = (char*)  malloc(8);
                    memset(str, '\0', 8);
                    sprintf(str, "%d\n", (int)_listeClients.size());
                    int lenMessage =0;
                    lenMessage = strlen(str) * sizeof(char);

                    ret = send(it->first, str, lenMessage, 0);
                    free(str);
                    if (ret == 0 || ret == -1)
                    {
                        break;
                    }

                }
            }
        }
        _mutexListeClient.unlock();
    }
}


