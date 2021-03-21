#ifndef PROTOCOLMESSAGING_H
#define PROTOCOLMESSAGING_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <list>

#include "ErrorHandler.h"
#include "LogRecords.h"

#define UDP_PORT_NUMBER "2001"
#define TCP_PORT_NUMBER "2000"

#define UDP_BUFFER_SIZE 1024

#define BACKLOG 10  //  Max of connections in queue



// Class Definitions:
class T3PResponse {
    public:
        T3PResponse();
        string statusCode;
        string statusMessage;
        list<string> dataList;
};


// Functions prototypes:

status_t parse_responce(string response);

    //UDP
    status_t settingUDPSocketListener(int *sockfd);
    status_t receiveUDPMessage(int from_sockfd);
    status_t sendUDPMessage(int to_sockfd);

    //TCP
    status_t settingTCPSocketListener(int *sockfd);
    status_t receiveTCPMessage(int from_sockfd);
    status_t sendTCPMessage(int to_sockfd);
    status_t acceptingTCPNewClient(int sockClient);

#endif