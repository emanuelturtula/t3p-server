#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <map>
#include <iostream>
#include "../headers/t3p_server.h"
#include "../headers/types.h" 
#include "../headers/udp_thread.h"
#include "../headers/tcp_thread.h"
#include "../headers/heartbeat_thread.h"
#include "../headers/referee_thread.h"

using namespace std;

extern map<string, int> config;

vector<Slot> slots;
MainDatabase mainDatabase;
vector<MatchEntry> matchDatabase; //We can have up to 50 players, so maximum matches are 25.

status_t get_bound_socket(const char *ip, int port, bool is_udp, int *sockfd);

status_t t3p_server(const char *ip)
{
    status_t status;
    int udpSocket;
    int tcpSocket;
    int connectedSocket;
    Logger logger;
    char buffer[TCP_BUFFER_SIZE];    
    int maxPlayers = config["MAX_PLAYERS_ONLINE"];
    int udpPort = config["UDP_PORT"];
    int tcpPort = config["TCP_PORT"];
    int tcpMaxPendingConnections = config["TCP_MAX_PENDING_CONNECTIONS"];

    slots.resize(maxPlayers);
    mainDatabase.resizeEntries(maxPlayers);
    matchDatabase.resize(maxPlayers/2);
    // Get the bound UDP socket.
    logger.debugMessage("Getting bound UDP socket...");
    if ((status = get_bound_socket(ip, udpPort, true, &udpSocket)) != STATUS_OK)
    {
        logger.errorHandler.printErrorCode(status);
        return status;
    }
    logger.debugMessage("OK.");

    // Get the bound TCP socket. 
    logger.debugMessage("Getting listening TCP socket...");
    if ((status = get_bound_socket(ip, tcpPort, false, &tcpSocket)) != STATUS_OK)
    {
        logger.errorHandler.printErrorCode(status);
        return status;
    }
    logger.debugMessage("OK.");

    // Now that we have both sockets listening, we open a new thread for managing 
    // the UDP messages, whilst in this thread we wait for a client to try a connection
    thread udp_thread(processUDPMesages, udpSocket, ip); 

    // Disable for testing other functions
    thread heartbeat_thread(heartbeatChecker);
    thread referee_thread(refereeProcess);
    logger.printMessage("Server is listening IP: " + string(ip), BOLDGREEN);
    while (1)
    {
        logger.debugMessage("Putting TCP socket in listening state...");
        if (listen(tcpSocket, tcpMaxPendingConnections) != 0)
        {
            close(tcpSocket);
            logger.debugMessage("TCP - Error listening");
            return ERROR_SOCKET_LISTENING;
        }
        logger.debugMessage("OK.");
        logger.debugMessage("Waiting for a client to connect...");
        if ((connectedSocket = accept(tcpSocket, NULL, NULL)) < 0)
        {
            cerr << "Error accepting client" << endl;
        }
        else 
        {
            logger.debugMessage("Accepted connection. Checking if there is a free slot...");
            bool isServerFull = true;
            for (int slotNumber = 0; slotNumber < slots.size(); slotNumber++)
            {
                if (slots[slotNumber].available)
                {
                    logger.debugMessage("Free slot found.");
                    slots[slotNumber].available = false;
                    logger.debugMessage("About to create a thread.");
                    thread clientThread(processClient, connectedSocket, slotNumber);
                    clientThread.detach();
                    logger.debugMessage("Thread created.");
                    isServerFull = false;
                    break;
                }
            }
            if (isServerFull)
            {
                logger.printMessage("T3P Server: a client is trying to connect but server is full");
                send(connectedSocket, "500|Server Error \r\n \r\n", strlen("500|Server Error \r\n \r\n"), 0);
                close(connectedSocket);
            }
        }
    }
    return STATUS_OK;
}

status_t get_bound_socket(const char *ip, int port, bool is_udp, int *sockfd)
{
    Logger logger;   
    int sock_type;
    int optval = 1;
    struct sockaddr_in server = {0};
    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Create the socket
    if (is_udp)
    {
        server.sin_addr.s_addr = INADDR_ANY;
        sock_type = SOCK_DGRAM;
    }
    else
    {
        server.sin_addr.s_addr = inet_addr(ip);
        sock_type = SOCK_STREAM;
    }
    if ((*sockfd = socket(AF_INET, sock_type, 0)) < 0) 
        return ERROR_SOCKET_CREATION;

    setsockopt((*sockfd), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // Bind the socket
    if (bind(*sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) 
    {
        close(*sockfd);
        return ERROR_SOCKET_BINDING;
    }
  
    return STATUS_OK;
}