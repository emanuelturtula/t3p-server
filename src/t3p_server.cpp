#include "t3p_server.h"



void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}




status_t t3p_server(void){

    status_t status;
    int sockUDPListener, sockTCPListener;
    int sockClient;
    LogRecords logHandler;
    ErrorHandler errorHandler;
    struct sigaction sa;


    logHandler.LogMessage("Setting UDPSocket Listener...");
    if( (status= settingUDPSocketListener(&sockUDPListener)) != STATUS_OK) {
        errorHandler.errorCode(status);
        return status;
    }
    logHandler.LogMessage("UDP socket Listener setted. Waiting to receive UDP packets in child process...");
    

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        status = ERROR_RIPPING_ALL_CHILD_PROCESSES;
        errorHandler.errorCode(status);
        return status;
    }

    // Opening child process for UDP messages handling
    if(!fork()){
        if( (status = serverDiscovery(sockUDPListener)) != STATUS_OK){
            errorHandler.errorCode(status);
            return status;
        }
        close(sockUDPListener);
        return status;
    }


    logHandler.LogMessage("Setting TCPSocket Listener...");
    if( (status= settingTCPSocketListener(&sockUDPListener)) != STATUS_OK) {
        errorHandler.errorCode(status);
        return status;
    }
    logHandler.LogMessage("TCP socket Listener setted. Waiting CLIENT to connect...");


    // Main Loop, where you receive all TCP connections of new clients:

    while(1){

        if( (status = acceptingTCPNewClient(sockClient)) != STATUS_OK){
            errorHandler.errorCode(status);
            return status;
        }
        
        // Opening child process for new Client/Player:
        if(!fork()){

            close(sockTCPListener); // Child doesn't need the listener

            if( (status = clientProcessing(sockClient)) != STATUS_OK){
                errorHandler.errorCode(status);
                return status;
            }
            close(sockClient);
            return status;
        }
        close(sockClient); // Parent doesn't need this

        return status; // ONLY FOR DEBBUGGING, COMMENT IF YOU ARE NOT DEBUGGING
    }

    return status;
}