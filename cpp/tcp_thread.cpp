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
    {ERROR_SERVER_ERROR, "500|Server Error \r\n \r\n"}
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


context_t lobbyContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t waitingResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t waitingOtherPlayerResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand, context_t context);
status_t parseMessage(string message, T3PCommand *t3pCommand);
status_t checkCommand(T3PCommand t3pCommand, context_t context);
status_t respond(int sockfd, status_t response);
status_t sendInviteFrom(int sockfd, string invitingPlayer);
status_t sendInvitationTimeout(int sockfd);
status_t sendInviteResponse(int sockfd, tcpcommand_t command);

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
    
    // Receive first message and save it formatted in a t3pcommand object. Also we check if the command is correct,
    // that is, if it is a known command and if its arguments are valid.
    if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        logger.errorHandler.printErrorCode(status);
    else
    {
        setsockopt(connectedSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        // If we could make it to here, it means the command was a login, the name was ok and the name was not taken. 
        // So we finally add the player to the database and change the context to lobby.
        context = LOBBY;
        entryNumber = mainDatabase.getAvailableEntry();
        playerEntry.context = context;
        playerEntry.playerName = t3pCommand.dataList.front();
        playerEntry.slotNumber = slotNumber;
        time(&(playerEntry.lastHeartbeat));
        mainDatabase.setEntry(entryNumber, playerEntry);
        respond(connectedSockfd, RESPONSE_OK);
    }

    bool heartbeat_expired = false; 
    // The first time we will enter here only if the context is LOBBY
    while ((context != SOCKET_CONNECTED) && (context != DISCONNECT) && (!heartbeat_expired))
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
                context = waitingOtherPlayerResponseContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case READY_TO_PLAY:
                while(1);
                break;
        }
    }

    if (context == SOCKET_CONNECTED)
    // If we got here after a wrong login, there won't be any entry, so we don't have to delete anything 
    // from the database. We have to tell the client that the login was incorrect
        respond(connectedSockfd, status);
    else 
        mainDatabase.clearEntry(entryNumber);
            
    // Close the socket
    close(connectedSockfd);
    // Previous ending the thread, we must free the slot.
    slots[slotNumber].clear();
}

context_t lobbyContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    Logger logger;
    context_t context = LOBBY;
    T3PCommand t3pCommand;
    tcpcommand_t command;
    MainDatabaseEntry myEntry;
    mainDatabase.setContext(entryNumber, context);
    myEntry = mainDatabase.getEntries()[entryNumber];
    string invitePlayer;

    while ((context == LOBBY))
    {
        t3pCommand.clear();
        // Check if heartbeat is expired
        if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
        {
            *heartbeat_expired = true;
            context = DISCONNECT;
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
        else
        {
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
                    // Check that the username is not myself    
                    if (myEntry.playerName == invitePlayer)
                        respond(connectedSockfd, ERROR_INCORRECT_NAME);
                    else 
                    {
                        mainDatabase.setInvitation(mainDatabase.getEntryNumber(invitePlayer), myEntry.playerName);
                        respond(connectedSockfd, RESPONSE_OK);
                        context = WAITING_OTHER_PLAYER_RESPONSE;
                        mainDatabase.setContext(entryNumber, context); 
                    }
                    break;
                case RANDOMINVITE:
                    // Random invite is similar to invite, with the only difference we choose a random player. 
                    // We need to review this because it is not random at all, but we don't wanna waste time
                    // here.
                    list<string> availablePlayers = mainDatabase.getAvailablePlayers(myEntry.playerName);
                    if (availablePlayers.empty())
                        respond(connectedSockfd, INFO_NO_PLAYERS_AVAILABLE);
                    else 
                    {
                        invitePlayer = availablePlayers.front();
                        mainDatabase.setInvitation(mainDatabase.getEntryNumber(invitePlayer), myEntry.playerName);
                        respond(connectedSockfd, RESPONSE_OK);
                        mainDatabase.setContext(entryNumber, context);
                        context = WAITING_OTHER_PLAYER_RESPONSE;
                    }
                    break;
            }
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
    tcpcommand_t command;
    MainDatabaseEntry myEntry;
    mainDatabase.setContext(entryNumber, context);
    myEntry = mainDatabase.getEntries()[entryNumber];
    // In this context we send INVITEFROM and wait for an answer. Possibilities are
    // decline, accept, or timeout
    sendInviteFrom(connectedSockfd, myEntry.invitingPlayerName);

    // We need to register this moment to later check if the invitation timed out.
    time(&invitation_time);
    while (context == WAITING_RESPONSE)
    {
        t3pCommand.clear();
        if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
        {
            *heartbeat_expired = true;
            context = DISCONNECT;
            break;
        }

        if ((time(NULL) - invitation_time) > INVITATION_SECONDS_TIMEOUT)
        {
            // If we entered here, it means the invitation timed out. So we first tell that 
            // to the client
            sendInvitationTimeout(connectedSockfd);
            // Then we need to set the invitation to Rejected
            myEntry = mainDatabase.getEntries()[entryNumber];
            myEntry.context = LOBBY;
            myEntry.invitationStatus = REJECTED;
            mainDatabase.setEntry(entryNumber, myEntry);
            context = LOBBY;
            break;
        }
        
        // Read incoming messages
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        else 
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    break;
                case ACCEPT:
                    myEntry = mainDatabase.getEntries()[entryNumber];
                    myEntry.invitationStatus = ACCEPTED;
                    myEntry.context = READY_TO_PLAY;
                    mainDatabase.setEntry(entryNumber, myEntry);
                    context = READY_TO_PLAY;
                    break;
                case DECLINE:
                    myEntry = mainDatabase.getEntries()[entryNumber];
                    myEntry.invitationStatus = REJECTED;
                    myEntry.context = LOBBY;
                    mainDatabase.setEntry(entryNumber, myEntry);
                    context = LOBBY;
                    break;
            }
        }
    }
    return context;
}

context_t waitingOtherPlayerResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    context_t context = WAITING_OTHER_PLAYER_RESPONSE;
    T3PCommand t3pCommand;
    status_t status;
    Logger logger;
    tcpcommand_t command;
    MainDatabaseEntry myEntry;
    mainDatabase.setContext(entryNumber, context);
    myEntry = mainDatabase.getEntries()[entryNumber];
    int invitedPlayerEntryNumber = mainDatabase.getInvitedPlayerEntryNumber(myEntry.playerName); 
    MainDatabaseEntry invitedPlayerEntry;

    while (context == WAITING_OTHER_PLAYER_RESPONSE)
    {
        if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
        {
            *heartbeat_expired = true;
            context = DISCONNECT;
            break;
        }
        t3pCommand.clear();
        // Read incoming messages
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        else
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    break;
            }
        }
        switch(mainDatabase.getEntries()[invitedPlayerEntryNumber].invitationStatus)
        {
            case REJECTED:
                myEntry = mainDatabase.getEntries()[entryNumber];
                invitedPlayerEntry = mainDatabase.getEntries()[invitedPlayerEntryNumber];
                myEntry.context = LOBBY;
                myEntry.readyToPlayWith = "";
                invitedPlayerEntry.invitationStatus = NONE;
                invitedPlayerEntry.invitingPlayerName = "";
                invitedPlayerEntry.readyToPlayWith = "";
                mainDatabase.setEntry(entryNumber, myEntry);
                mainDatabase.setEntry(entryNumber, invitedPlayerEntry);
                context = LOBBY;
                sendInviteResponse(connectedSockfd, DECLINE);
                break;
            case PENDING:
                break; 
            case ACCEPTED:
                myEntry = mainDatabase.getEntries()[entryNumber];
                invitedPlayerEntry = mainDatabase.getEntries()[invitedPlayerEntryNumber];
                myEntry.context = READY_TO_PLAY;
                myEntry.readyToPlayWith = invitedPlayerEntry.playerName;
                invitedPlayerEntry.invitationStatus = NONE;
                invitedPlayerEntry.invitingPlayerName = "";
                invitedPlayerEntry.readyToPlayWith = myEntry.playerName;
                mainDatabase.setEntry(entryNumber, myEntry);
                mainDatabase.setEntry(entryNumber, invitedPlayerEntry);
                context = READY_TO_PLAY;
                sendInviteResponse(connectedSockfd, ACCEPT);
                break;
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
    list<string> availablePlayers;
    tcpcommand_t command;
    // First we need to check if the incomming command is present
    // in a list of possible commands.
    if (find(TCPCommands.begin(), TCPCommands.end(), t3pCommand.command) == TCPCommands.end())
        return ERROR_BAD_REQUEST;

    // Translate the command to a tcpcommand type to ease the later code
    command = TCPCommandTranslator[t3pCommand.command];
    // Now that we know that the command actually exists, we need 
    // to check based on the context if it is valid.
    switch(context)
    {
        case SOCKET_CONNECTED:
            switch(command)
            {
                case LOGIN:
                    playerName = t3pCommand.dataList.front();
                    if ((status = checkPlayerName(playerName)) != STATUS_OK)
                        return status;
                    if (checkPlayerIsOnline(playerName))
                        return ERROR_NAME_TAKEN;
                    break;
                default:
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }            
            break;
        case LOBBY:
            switch(command)
            {
                case HEARTBEAT:
                    break;
                case INVITE:
                    playerName = t3pCommand.dataList.front();
                    if ((status = checkPlayerName(playerName)) != STATUS_OK)
                        return status;
                    if (!checkPlayerIsOnline(playerName))
                        return ERROR_PLAYER_NOT_FOUND;
                    if (!checkPlayerIsAvailable(playerName))
                        return ERROR_PLAYER_OCCUPIED;
                    break;
                case RANDOMINVITE:
                    availablePlayers = mainDatabase.getAvailablePlayers();
                    if (availablePlayers.empty())
                        return INFO_NO_PLAYERS_AVAILABLE;
                    break;
                case LOGOUT:
                    break;
                default:
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }
            break;
        case WAITING_OTHER_PLAYER_RESPONSE:
            switch(command)
            {
                case HEARTBEAT:
                    break;
                default: 
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }
            break;
        case WAITING_RESPONSE:
            switch(command)
            {
                case HEARTBEAT:
                    break;
                case ACCEPT:
                    break;
                case DECLINE:
                    break;
                default:
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }
        default:
            return ERROR_SERVER_ERROR;
    }
    return STATUS_OK;
}

status_t respond(int sockfd, status_t response)
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

status_t sendInviteResponse(int sockfd, tcpcommand_t command)
{
    string message;
    const char *c_message;
    switch(command)
    {
        case ACCEPT:
            message = "ACCEPT \r\n \r\n";
            break;
        case DECLINE:
            message = "DECLINE \r\n \r\n";
            break;
    }
    c_message = message.c_str();
    if (send(sockfd, c_message, strlen(c_message), 0) < 0)
        return ERROR_SENDING_MESSAGE;
    return STATUS_OK;
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