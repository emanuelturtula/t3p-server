#ifndef MAIN_H
#define MAIN_H

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
#include "SocketsSetter.h"
//#include "LogRecords.h"



using namespace std;

// Create a global log and errorHandler:

ErrorHandler errorHandler;


#endif