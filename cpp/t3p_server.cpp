#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <thread>
#include "../headers/t3p_server.h"
#include "../headers/types.h" 
#include "../headers/udp_thread.h"
#include "../headers/tcp_thread.h"
#include "../headers/config.h"

status_t get_bound_socket(const char *ip, int port, bool is_udp, int *sockfd);

status_t t3p_server(const char *ip)
{
    status_t status;
    int udpSocket;
    int tcpSocket;
    int connectedSocket;
    Logger logger;

    // Get the bound UDP socket.
    logger.debugMessage("Getting binded UDP socket...");
    if ((status = get_bound_socket(ip, UDP_PORT_NUMBER, true, &udpSocket)) != STATUS_OK)
    {
        logger.errorHandler.printErrorCode(status);
        return status;
    }
    logger.debugMessage("OK.");

    // Get the bound TCP socket. 
    logger.debugMessage("Getting listening TCP socket...");
    if ((status = get_bound_socket(ip, TCP_PORT_NUMBER, false, &tcpSocket)) != STATUS_OK)
    {
        logger.errorHandler.printErrorCode(status);
        return status;
    }
    logger.debugMessage("OK.");

    // Now that we have both sockets listening, we open a new thread for managing 
    // the UDP messages, whilst in this thread we wait for a client to try a connection
    thread udp_thread(processUDPMesages, udpSocket); 

    while (1)
    {
        logger.debugMessage("Putting TCP socket in listening state...");
        if (listen(tcpSocket, TCP_MAX_PENDING_CONNECTIONS) != 0)
        {
            close(tcpSocket);
            logger.debugMessage("TCP - Error listening");
            // error handling
            return ERROR_SOCKET_LISTENING;
        }
        logger.debugMessage("OK.");
        logger.debugMessage("Waiting for a client to connect...");
        if ((connectedSocket = accept(tcpSocket, NULL, NULL)) < 0)
        {
            // error handling
        }
        // create a new thread. 
        // How are we going to manage the vector of threads?
    
    }
    


    

    return status;
}

status_t get_bound_socket(const char *ip, int port, bool is_udp, int *sockfd)
{
    Logger logger;   
    int sock_type;
    struct sockaddr_in server = {0};

    if (sockfd == NULL)
        return ERROR_NULL_POINTER;

    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Create the socket
    if (is_udp)
        sock_type = SOCK_DGRAM;
    else
        sock_type = SOCK_STREAM;

    if ((*sockfd = socket(AF_INET, sock_type, 0)) < 0) 
    { 
        logger.errorHandler.printErrorCode(ERROR_SOCKET_CREATION);
        logger.printMessage(ERROR_SOCKET_CREATION); 
        return ERROR_SOCKET_CREATION;
    }

    // Bind the socket
    if (bind(*sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) 
    {
        close(*sockfd);
        logger.errorHandler.printErrorCode(ERROR_SOCKET_BINDING); 
        return ERROR_SOCKET_BINDING;
    }

    // if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) < 0) 
    //     return ERROR_SOCKET_CONFIGURATION;
  
    return STATUS_OK;
}