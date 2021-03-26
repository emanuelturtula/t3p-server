#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include "../headers/tcp_thread.h"
#include "../headers/types.h"

using namespace std;

// Global variables defined in another file
extern vector<Slot> slots;
extern MainDatabase mainDatabase;

list<string> TCPCommands = {
    "LOGIN",
    "LOGOUT",
    "INVITE",
    "RANDOMINVITE",
    "MARKSLOT",
    "GIVEUP"
};


status_t receiveMessage(int sockfd, T3PCommand *t3pCommand);
status_t parseMessage(string message, T3PCommand *t3pCommand);
status_t respond(int sockfd, status_t response);
status_t checkCommand(T3PCommand t3pCommand, context_t context);
void clearSlot(int slotNumber);

void processClient(int connectedSockfd, int slotNumber)
{
    status_t status;
    context_t context;
    T3PCommand t3pCommand;
    Logger logger;

    // Receive first message and save it formatted in a t3pcommand object
    if ((status = receiveMessage(connectedSockfd, &t3pCommand)) != STATUS_OK)
    {
        //Don't know if logging this is useful for us
        logger.errorHandler.printErrorCode(status);
        // Respond corresponding status
        while (respond(connectedSockfd, status) != STATUS_OK);
        // Close the socket
        close(connectedSockfd);  
        // Clear the slot taken     
        clearSlot(slotNumber);
        return;
    }

    // We want now to check if the command was a login and if the player name
    // is compliant.
    if ((status = checkCommand(t3pCommand, context)) != STATUS_OK)
    {
        //Don't know if logging this is useful for us
        logger.errorHandler.printErrorCode(status);
        // Respond corresponding status
        while (respond(connectedSockfd, status) != STATUS_OK);
        // Close the socket
        close(connectedSockfd);  
        // Terminate the thread      
        return;
    }

    // If we could make it to here, it means the command was a login and the name was ok.
    // Finally, we check if the player name is not taken. dataList from t3pcommand object
    // should only have one item (the player name).
    bool name_available = true;
    for (auto const& playerOnline : mainDatabase.getPlayersOnline())
    {
        if (t3pCommand.dataList.front() == playerOnline)
        {
            name_available = false;
            break;
        }
    } 

    if (!name_available)
    {
        respond(connectedSockfd, status);
    }    

    // Send response to client
    


    //The first thing to do, is to check that the login is correct or the player is not online.
    //If  

    //Previous ending the thread, we must free the slot.
}

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand)
{
    char message[TCP_BUFFER_SIZE] = {0};
    if (recv(sockfd, message, sizeof(message), NULL) < 0)
        return ERROR_SERVER_ERROR;
    if (parseMessage(string(message), t3pCommand) != STATUS_OK)
        return ERROR_BAD_REQUEST;
    return STATUS_OK;    
}

status_t parseMessage(string message, T3PCommand *t3pCommand)
{
    size_t pos;
    if ((pos = message.rfind(" \r\n \r\n")) == string::npos)
        return ERROR_BAD_REQUEST;
    
    // Strip last \r\n
    message.erase(pos+3);

    // If message contains "|", grab the first part as a command
    if ((pos = message.find("|")) != string::npos)
    {
        t3pCommand->command = message.substr(0, pos);
        message.erase(0, pos+1);
        while ((pos = message.find(" \r\n")) != string::npos)
        {
            t3pCommand->dataList.push_back(message.substr(0, pos));
            message.erase(0, pos+3);
        }
    }
    // else, the message should only contain the command
    else 
    {
        //I must have it because we checked at the beginning
        pos = message.find(" \r\n");
        t3pCommand->command = message.substr(0, pos);
    }
        
    return STATUS_OK;
}

status_t respond(int sockfd, status_t response)
{
    return STATUS_OK;
}


/**
 * Check if a received command is valid for the given context.
 *
 * @param t3pCommand T3PCommand object containing the command and eventually the values.
 * @param context   context_t object containing the current context
 * @return status_t object with the result of the checker.
 */
status_t checkCommand(T3PCommand t3pCommand, context_t context)
{
    //First we need to check if the incomming command is present
    //in a list of possible commands.
    if (find(TCPCommands.begin(), TCPCommands.end(), t3pCommand.command) == TCPCommands.end())
        return ERROR_BAD_REQUEST;

    // Now that we know that the command actually exists, we need 
    // to check based on the context if it is valid.
    switch(context)
    {
        // When socket has just been connected, the only possible command is LOGIN 
        case SOCKET_CONNECTED:
            if (t3pCommand.command != "LOGIN")
                return ERROR_COMMAND_OUT_OF_CONTEXT;
            // TODO: Check player's name
            break;
        case LOBBY:


    }
    return STATUS_OK;
}


void clearSlot(int slotNumber)
{
    slots[slotNumber] = Slot();
}