#pragma once

#include <string>
#include <list>

using namespace std;

enum status_t {
    // Internal server Errors:
    STATUS_OK = 0,
    ERROR_NULL_POINTER,
    ERROR_SOCKET_CREATION,
    ERROR_SOCKET_CONFIGURATION,
    ERROR_SOCKET_LISTENING,
    ERROR_GETTING_ADDRINFO,
    ERROR_SOCKET_BINDING,
    ERROR_UDPLISTENER_GETADDRINFO,
    ERROR_UDPLISTENER_SOCKET_CREATION, //Not Disruptive
    ERROR_UDPLISTENER_SOCKET_SETSOCKOPT,
    ERROR_UDPLISTENER_SOCKET_BINDING, //Not Disruptive
    ERROR_UDPLISTENER_SOCKET_BINDING_FAILURE,
    ERROR_UDPLISTENER_SOCKET_LISTEN_FAILURE,
    ERROR_RIPPING_ALL_CHILD_PROCESSES,


    /*******************************/
    // RFC Responses:
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
    ERROR_SERVER_ERROR = 500
};

class ErrorHandler {
    public:
        ErrorHandler();
        void printErrorCode(status_t);
};

class Printer {
    public:
        Printer();
        void writeStdout(string message);
        void writeStderr(string message);
};

class Logger {
    public:
        Logger();
        ErrorHandler errorHandler;
        void printMessage(status_t);
        void printMessage(string);
        void printMessage(string, status_t);
        void debugMessage(string);
    private:
        Printer printer;
};

class T3PResponse {
    public:
        T3PResponse();
        string statusCode;
        string statusMessage;
        list<string> dataList;
};