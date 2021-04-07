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
    "HEARTBEAT",
    "ACCEPT",
    "DECLINE",
    "RANDOMINVITE",
    "MARKSLOT",
    "GIVEUP"
};

enum tcpcommand_t {
    LOGIN,
    LOGOUT,
    INVITE,
    HEARTBEAT,
    ACCEPT,
    DECLINE,
    RANDOMINVITE,
    MARKSLOT,
    GIVEUP
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
    {ERROR_SERVER_ERROR, "500|Server Error \r\n \r\n"},
    {ACCEPT, "ACCEPT \r\n \r\n"},
    {DECLINE, "DECLINE \r\n \r\n"}
};

map<string, tcpcommand_t> TCPCommandTranslator = {
    {"LOGIN", LOGIN},
    {"LOGOUT", LOGOUT},
    {"INVITE", INVITE},
    {"HEARTBEAT", HEARTBEAT},
    {"ACCEPT", ACCEPT},
    {"DECLINE", DECLINE},
    {"RANDOMINVITE", RANDOMINVITE},
    {"MARKSLOT", MARKSLOT},
    {"GIVEUP", GIVEUP}
};

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand, context_t context);
status_t parseMessage(string message, T3PCommand *t3pCommand);
status_t respond(int sockfd, status_t response, string args[] = NULL);
status_t sendInvitation(int sockfd, string invitingPlayer);
status_t checkCommand(T3PCommand t3pCommand, context_t context);
void clearSlot(int slotNumber);
void clearEntry(int entryNumber);
status_t checkPlayerName(string name);
bool checkPlayerIsOnline(string name);
bool checkPlayerIsAvailable(string name);

//Main function
void processClient(int connectedSockfd, int slotNumber)
{
    status_t status;
    context_t context = SOCKET_CONNECTED;
    T3PCommand t3pCommand;
    Logger logger;
    MainDatabaseEntry playerEntry;
    time_t timeout;
    int entryNumber;

    // Set the socket reception timeout to 1 second
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(connectedSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // Receive first message and save it formatted in a t3pcommand object. Also we check if the command is correct,
    // that is, if it is a known command and if its arguments are valid.
    if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        logger.errorHandler.printErrorCode(status);
    else
    {
        // If we could make it to here, it means the command was a login, the name was ok and the name was not taken. 
        // So we finally add the player to the database and change the context to lobby.
        context = LOBBY;
        entryNumber = mainDatabase.getAvailableEntry();
        playerEntry.context = context;
        playerEntry.playerName = t3pCommand.dataList.front();
        playerEntry.slotNumber = slotNumber;
        time(&(playerEntry.lastHeartbeat));
        mainDatabase.setEntry(entryNumber, playerEntry);
        while (respond(connectedSockfd, RESPONSE_OK) != STATUS_OK);
    }

    bool heartbeat_expired = false; 
    // The first time we will enter here only if the context is LOBBY
    while ((context != SOCKET_CONNECTED) && (context != LOGOUT) && (!heartbeat_expired))
    {
        switch(context)
        {
            case LOBBY:
                context = lobbyContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case WAITING_RESPONSE:
                context = waitingResponseContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case WAITING_OTHER_PLAYER_RESPONSE:
                break;
        }
        
        while (context == WAITING_OTHER_PLAYER_RESPONSE)
        {
            if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
            {
                heartbeat_expired = true;
                break;
            }
            t3pCommand.clear();
            // Read incoming messages
            if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
            {
                //Don't know if logging this is useful for us
                logger.errorHandler.printErrorCode(status);
                // Respond corresponding status
                while (respond(connectedSockfd, status) != STATUS_OK);
            }
            // If it is a Heartbeat
            if (t3pCommand.command == "HEARTBEAT")
                // When a heartbeat arrives, we only update our entry.
                mainDatabase.udpateHeartbeat(entryNumber);
        }
    }

    if (context == SOCKET_CONNECTED)
    // If we got here after a wrong login, there won't be any entry, so we don't have to delete anything 
    //from the database. We have to tell the client that the login was incorrect
        respond(connectedSockfd, status);
    else 
        clearEntry(entryNumber);
            
    // Close the socket
    close(connectedSockfd);
    // Previous ending the thread, we must free the slot.
    clearSlot(slotNumber);
}

context_t lobbyContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    Logger logger;
    context_t context = LOBBY;
    T3PCommand t3pCommand;
    tcpcommand_t command;
    MainDatabaseEntry playerEntry;
    
    playerEntry = mainDatabase.getEntries()[entryNumber];
    string invitePlayer;

    while ((context == LOBBY))
    {
        t3pCommand.clear();
        // Check if heartbeat is expired
        if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
        {
            *heartbeat_expired = true;
            break;
        }
        // Check if we have invitation pending
        if (mainDatabase.getEntries()[entryNumber].invitationStatus == PENDING)
        {
            context = WAITING_RESPONSE;
            break;
        }
        // Receive a message (wait only one second)
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
            respond(connectedSockfd, status);
        // Translate the command string to a command enum
        command = TCPCommandTranslator[t3pCommand.command];
        switch(command)
        {
            case HEARTBEAT:
                // If it is a heartbeat, update the entry
                mainDatabase.udpateHeartbeat(entryNumber);
                break;
            case INVITE:
                // If it is an invite, we already checked the format and that the player is available.
                invitePlayer = t3pCommand.dataList.front();
                mainDatabase.setInvitation(mainDatabase.getEntryNumber(invitePlayer), playerEntry.playerName);
                respond(connectedSockfd, RESPONSE_OK);
                mainDatabase.setContext(entryNumber, context); 
                context = WAITING_OTHER_PLAYER_RESPONSE;
                break;
            case RANDOMINVITE:
                // Random invite is similar to invite, with the only difference we choose a random player. 
                // We need to review this because it is not random at all, but we don't wanna waste time
                // here.
                list<string> availablePlayers = mainDatabase.getAvailablePlayers();
                invitePlayer = availablePlayers.front();
                mainDatabase.setInvitation(mainDatabase.getEntryNumber(invitePlayer), playerEntry.playerName);
                respond(connectedSockfd, RESPONSE_OK);
                mainDatabase.setContext(entryNumber, context);
                context = WAITING_OTHER_PLAYER_RESPONSE;
                break;
        }
    }
    return context;
}

context_t waitingResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    Logger logger;
    context_t context = WAITING_RESPONSE;
    T3PCommand t3pCommand;
    time_t invitation_time;
    // In this context we send INVITEFROM and wait for an answer. Possibilities are
    // decline, accept, or timeout
    sendInviteFrom(connectedSockfd, mainDatabase.getEntries()[entryNumber].invitingPlayerName);

    // We need to register this moment to later check if the invitation timed out.
    time(&invitation_time);
    while (context == WAITING_RESPONSE)
    {
        t3pCommand.clear();
        if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
        {
            *heartbeat_expired = true;
            break;
        }

        if (time(NULL) - invitation_time > INVITATION_SECONDS_TIMEOUT)
        {
            // If we entered here, it means the invitation timed out. So we first tell that 
            // to the client
            sendInvitationTimeout(connectedSockfd);
            // Then we need to set the invitation to Rejected
            mainDatabase.setInvitation(entryNumber, REJECTED);
            context = LOBBY;
            mainDatabase.setContext(entryNumber, context);
            break;
        }
        // TODO: add timeout check.
        // This context will only be access if the referee threads puts this thread in this context 
        
        // Read incoming messages
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            //Don't know if logging this is useful for us
            logger.errorHandler.printErrorCode(status);
            // Respond corresponding status
            while (respond(connectedSockfd, status) != STATUS_OK);
        }
        if (t3pCommand.command == "HEARTBEAT")
            // When a heartbeat arrives, we only update our entry.
            mainDatabase.udpateHeartbeat(entryNumber);
        else if (t3pCommand.command == "ACCEPT")
        {
            // Do sth.
        }
        else if (t3pCommand.command == "DECLINE")
        {
            // Do sth. Go back to lobby and clear the invitation from 
        }
    }
    return context;
}

/**
 * Parse a message and put the result in a T3PCommand object.
 *
 * @param sockfd int containing the connected TCP socket.
 * @param *t3pCommand Pointer to a T3PCommand object where the parsed data will be written.
 * @param context context_t object containing the current context. It is used to check if the command is valid given a known context.
 * @return status_t object with the result of the receiver.
 */
status_t receiveMessage(int sockfd, T3PCommand *t3pCommand, context_t context)
{
    status_t status;
    char message[TCP_BUFFER_SIZE] = {0};
    int bytes = recv(sockfd, message, sizeof(message), 0);
    if (bytes > 0)
    {
        if (parseMessage(string(message), t3pCommand) != STATUS_OK)
            return ERROR_BAD_REQUEST;
        if ((status = checkCommand(*t3pCommand, context)) != STATUS_OK)
            return status;
    }
    return STATUS_OK;    
}

/**
 * Parse a message and put the result in a T3PCommand object.
 *
 * @param message String containing the message to parse. It has to be valid according to RFC documentation.
 * @param *t3pCommand Pointer to a T3PCommand object where the parsed data will be written.
 * @return status_t object with the result of the parser.
 */
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

/**
 * Check if a received command is valid for the given context.
 *
 * @param t3pCommand T3PCommand object containing the command and eventually the values.
 * @param context   context_t object containing the current context.
 * @return status_t object with the result of the checker.
 */
status_t checkCommand(T3PCommand t3pCommand, context_t context)
{
    status_t status;
    string playerName;
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
            // If command is not a login, then it's out of context
            if (t3pCommand.command != "LOGIN")
                return ERROR_COMMAND_OUT_OF_CONTEXT;
            playerName = t3pCommand.dataList.front();
            // Check if the player's name is compliant
            if ((status = checkPlayerName(playerName)) != STATUS_OK)
                return status;
            // Check if there's a player online with that name. If it is,
            // then the result is that the name is taken.
            if (checkPlayerIsOnline(playerName))
                return ERROR_NAME_TAKEN;
            break;
        // When a player is in the lobby, it can only invite or logout
        case LOBBY:
            if ((t3pCommand.command != "HEARTBEAT") &&
                (t3pCommand.command != "INVITE") && 
                (t3pCommand.command != "RANDOMINVITE") && 
                (t3pCommand.command != "LOGOUT"))
                return ERROR_COMMAND_OUT_OF_CONTEXT;
            if (t3pCommand.command == "INVITE")
            {
                playerName = t3pCommand.dataList.front();
                if ((status = checkPlayerName(playerName)) != STATUS_OK)
                    return status;
                if (!checkPlayerIsOnline(playerName))
                    return ERROR_PLAYER_NOT_FOUND;
                if (!checkPlayerIsAvailable(playerName))
                    return ERROR_PLAYER_OCCUPIED;
            }
            if (t3pCommand.command == "RANDOMINVITE")
            {
                // If there are not available players, then we return 
                // that as an info
                list<string> availablePlayers = mainDatabase.getAvailablePlayers();
                if (availablePlayers.empty())
                    return INFO_NO_PLAYERS_AVAILABLE;
            }
            break;
            // When a player is waiting for another players response, it can only send
            // heartbeats.
        case WAITING_OTHER_PLAYER_RESPONSE:
            if (t3pCommand.command != "HEARTBEAT")
                return ERROR_COMMAND_OUT_OF_CONTEXT;
            break;
        case WAITING_RESPONSE:
            if ((t3pCommand.command != "HEARTBEAT") &&
                (t3pCommand.command != "ACCEPT") &&
                (t3pCommand.command != "DECLINE"))
                return ERROR_COMMAND_OUT_OF_CONTEXT;
        default:
            return ERROR_SERVER_ERROR;
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

status_t sendInviteFrom(int sockfd, string invitingPlayer)
{
    string message = "INVITEFROM|";
    message += invitingPlayer;
    message += " \r\n \r\n";
    const char *c_message = message.c_str();
    if (send(sockfd, c_message, strlen(c_message), 0) < 0)
        return ERROR_SENDING_MESSAGE;
    return STATUS_OK;
}

status_t sendInvitationTimeout(int sockfd)
{
    string message = "INVITATIONTIMEOUT \r\n \r\n";
    const char *c_message = message.c_str();
    if (send(sockfd, c_message, strlen(c_message), 0) < 0)
        return ERROR_SENDING_MESSAGE;
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

bool checkPlayerIsOnline(string name)
{
    for (auto const& playerOnline : mainDatabase.getPlayersOnline())
    {
        if (name == playerOnline)
            return true;
    } 
    return false;
}

bool checkPlayerIsAvailable(string name)
{
    for (auto const& playerOnline : mainDatabase.getAvailablePlayers())
    {
        if (name == playerOnline)
            return true;
    } 
    return false;
}