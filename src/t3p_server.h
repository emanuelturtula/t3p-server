#ifndef T3P_SERVER_H
#define T3P_SERVER_H

// Std Libs:
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

// Created libs:
#include "ErrorHandler.h"
#include "LogRecords.h"
#include "ProtocolMessaging.h"
#include "ServerDiscovery.h"
#include "clientProcessing.h"



using namespace std;


status_t t3p_server(void);


#endif