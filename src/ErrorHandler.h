#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <iostream>
#include <string.h>

//#include "LogRecords.h"

using namespace std;

//Not Disruptive: means that there was an error in a function, but can continue
typedef enum{
    // Internal server Errors:
    STATUS_OK = 0,
    ERROR_UDPLISTENER_SOCKET_PARAMETER_NULL,
    ERROR_UDPLISTENER_GETADDRINFO,
    ERROR_UDPLISTENER_SOCKET_CREATION, //Not Disruptive
    ERROR_UDPLISTENER_SOCKET_SETSOCKOPT,
    ERROR_UDPLISTENER_SOCKET_BINDING, //Not Disruptive
    ERROR_UDPLISTENER_SOCKET_BINDING_FAILURE,
    ERROR_UDPLISTENER_SOCKET_LISTEN_FAILURE,
    ERROR_RIPPING_ALL_CHILD_PROCESSES,


    /*******************************/
    // RFC Errors:
    //1xx: Informative responses
    INFO_NO_PLAYERS_AVAILABLE = 100,
    //2xx: Correct petitions
    RESPONSE_OK = 200,
    //4xx: Errors from client
    ERROR_BAD_REQUEST = 400,
    ERROR_INCORRECT_NAME = 401,
    ERROR_NAME_TAKEN = 402,
    ERROR_PLAYER_NOT_FOUND = 403,
    ERROR_PLAYER_OCCUPIED = 404,
    ERROR_BAD_SLOT = 405,
    ERROR_NOT_TURN = 406,
    ERROR_INVALID_COMMAND = 407,
    ERROR_COMMAND_OUT_OF_CONTEXT = 408,
    ERROR_CONNECTION_LOST = 409,
    //5xx: Errors from server
    SRVERROR_SERVER_ERROR = 500


} status_t;


class ErrorHandler {
    public:
        ErrorHandler();
        void errorCode(status_t);
};


#endif