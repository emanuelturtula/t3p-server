#ifndef SOCKETSSETTER_H
#define SOCKETSSETTER_H

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

#include "ErrorHandler.h"
#include "LogRecords.h"

#define UDP_PORT_NUMBER "2001"
#define TCP_PORT_NUMBER "2000"

#define BACKLOG 10  //  Max of connections in queue

status_t settingUDPSocketListener(int *sockfd);

#endif