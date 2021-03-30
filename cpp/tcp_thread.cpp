#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <regex>
#include <ctime>
#include <map>
#include "../headers/tcp_thread.h"
#include "../headers/types.h"

using namespace std;

// Global variables defined in another file
extern vector<Slot> slots;
extern MainDatabase mainDatabase;

// List of possible TCP commands
list<string> TCPCommands = {
    "LOGIN",
    "LOGOUT",
    "INVITE",
    "RANDOMINVITE",
    "MARKSLOT",
    "GIVEUP"
};

// Dictionary of responses
map<status_t, string> TCPResponseDictionary = {
        {INFO_NO_PLAYERS_AVAILABLE, "101|No players available \r\n \r\n"},
        {RESPONSE_OK, "200|OK \r\n \r\n"},
        {ERROR_BAD_REQUEST, "400|Bad Request \r\n \r\n"},
        {ERROR_INCORRECT_NAME, "401|Incorrect Name \r\n \r\n"},
        {ERROR_NAME_TAKEN, "402|Name Taken \r\n \r\n"},
        {ERROR_PLAYER_NOT_FOUND, "403|Player not Found \r\n \r\n"},
        {ERROR_PLAYER_OCCUPIED, "404|Player Occupied \r\n \r\n"},
        {ERROR_BAD_SLOT, "405|Bad Slot \r\n \r\n"},
        {ERROR_NOT_TURN, "406|Not Turn"},
        {ERROR_INVALID_COMMAND, "407|Invalid command \r\n \r\n"},
        {ERROR_COMMAND_OUT_OF_CONTEXT, "408|Command out of context \r\n \r\n"},
        {ERROR_CONNECTION_LOST, "409|Connection Lost \r\n \r\n"},
        {ERROR_SERVER_ERROR, "500|Server Error \r\n \r\n"}
};

//Prototypes
status_t receiveMessage(int sockfd, T3PCommand *t3pCommand);
status_t parseMessage(string message, T3PCommand *t3pCommand);
status_t respond(int sockfd, status_t response, string args[] = NULL);
status_t checkCommand(T3PCommand t3pCommand, context_t context);
void clearSlot(int slotNumber);
void clearEntry(int entryNumber);
status_t checkPlayerName(string name);

//Main function
void processClient(int connectedSockfd, int slotNumber)
{
    status_t status;
    context_t context;
    T3PCommand t3pCommand;
    Logger logger;
    MainDatabaseEntry playerEntry;

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
        clearSlot(slotNumber);   
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
        while (respond(connectedSockfd, ERROR_NAME_TAKEN) != STATUS_OK);

    // The name is available, so we finally add the player to the database and login
    context = LOBBY;
    int entryNumber = mainDatabase.getAvailableEntry();
    playerEntry.context = context;
    playerEntry.playerName = t3pCommand.dataList.front();
    playerEntry.slotNumber = slotNumber;
    time(&(playerEntry.lastHeartbeat));
    mainDatabase.setEntry(entryNumber, playerEntry);
    while (respond(connectedSockfd, RESPONSE_OK) != STATUS_OK);

    // Now the player is in the LOBBY. 

    // Close the socket
    close(connectedSockfd);
    //Previous ending the thread, we must free the slot.
    clearSlot(slotNumber);
    clearEntry(entryNumber);
    return;
}

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand)
{
    char message[TCP_BUFFER_SIZE] = {0};
    if (recv(sockfd, message, sizeof(message), 0) < 0)
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

status_t respond(int sockfd, status_t response, string args[])
{
    string message = TCPResponseDictionary[response];
    const char *c_message = message.c_str(); 
    if (send(sockfd, c_message, strlen(c_message), 0) < 0)
        return ERROR_SENDING_MESSAGE;
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
    status_t status;
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
            if ((status = checkPlayerName(t3pCommand.dataList.front())) != STATUS_OK)
                return status;
            break;
        case LOBBY:
            if ((t3pCommand.command != "INVITE") || 
                (t3pCommand.command != "RANDOMINVITE") || 
                (t3pCommand.command != "LOGOUT"))
                return ERROR_COMMAND_OUT_OF_CONTEXT;
            if (t3pCommand.command == "INVITE")
            {
                if ((status = checkPlayerName(t3pCommand.dataList.front())) != STATUS_OK)
                    return status;
            }
            break;
        // TODO: Add other contexts 
        default:
            return ERROR_SERVER_ERROR;
    }
    return STATUS_OK;
}

void clearEntry(int entryNumber)
{
    MainDatabaseEntry entry;
    mainDatabase.setEntry(entryNumber, entry);
}

void clearSlot(int slotNumber)
{
    slots[slotNumber].available = true;
    //We don care about clearing the thread id in the slots vector. Just knowing
    //the slot is available is good enough
}

status_t checkPlayerName(string name)
{
    regex nameChecker("^[a-zA-Z]+$");
    if ((name.length() < 3) || (name.length() > 20))
        return ERROR_INCORRECT_NAME;
    if (!regex_match(name, nameChecker))
        return ERROR_INCORRECT_NAME;
    return STATUS_OK;
}