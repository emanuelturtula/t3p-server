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
extern vector<MatchEntry> matchDatabase;

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
    "GIVEUP",
    "200",
    "400"
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
    GIVEUP,
    OK,
    BAD
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
    {"GIVEUP", GIVEUP},
    {"200", OK},
    {"400", BAD}
};


context_t lobbyContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t waitingResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t waitingOtherPlayerResponseContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t readyToPlayContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);
context_t matchContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired);

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand, context_t context);
status_t peek_tcp_buffer(int sockfd, int *read_bytes, string *socket_message);
status_t parseMessage(string message, T3PCommand *t3pCommand);
status_t checkCommand(T3PCommand t3pCommand, context_t context);
status_t respond(int sockfd, status_t response);
status_t sendInviteFrom(int sockfd, string invitingPlayer);
status_t sendInvitationTimeout(int sockfd);
status_t sendInviteResponse(int sockfd, tcpcommand_t command);
status_t sendTurnMessage(int sockfd, int matchEntryNumber, bool myTurn);
status_t sendMatchEnd(int sockfd, string myName, int matchEntryNumber);
status_t sendMessage(int sockfd, string message);

status_t checkPlayerName(string name);
bool checkPlayerIsOnline(string name);
bool checkPlayerIsAvailable(string name);

bool isHeartbeatExpired(bool *heartbeat_expired, int entryNumber);

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

    // Set the socket reception timeout to LOGIN_SECONDS_TIMEOUT to wait for a login
    struct timeval tv;
    tv.tv_sec = LOGIN_SECONDS_TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(connectedSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // Receive first message and save it formatted in a t3pcommand object. Also we check if the command is correct,
    // that is, if it is a known command and if its arguments are valid.
    if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        logger.errorHandler.printErrorCode(status);
    if (t3pCommand.isNewCommand)
    {
        // Change the reception timeout to 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(connectedSockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        // If we could make it to here, it means the command was a login, the name was ok and the name was not taken. 
        // So we finally add the player to the database and change the context to lobby.
        context = LOBBY;
        entryNumber = mainDatabase.getAvailableEntry();
        playerEntry.context = context;
        playerEntry.playerName = t3pCommand.dataList.front();
        playerEntry.slotNumber = slotNumber;
        playerEntry.connectedSockfd = connectedSockfd;
        time(&(playerEntry.lastHeartbeat));
        mainDatabase.setEntry(entryNumber, playerEntry);
        respond(connectedSockfd, RESPONSE_OK);
        logger.printMessage("Client process: " + playerEntry.playerName + " connected.");
    }

    bool heartbeat_expired = false; 
    // The first time we will enter here only if the context is LOBBY
    while ((context != SOCKET_CONNECTED) && (context != DISCONNECT) && (!heartbeat_expired))
    {
        switch(context)
        {
            case LOBBY:
                logger.printMessage("Client process: " + playerEntry.playerName + " is in Lobby.");
                context = lobbyContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case WAITING_RESPONSE:
                logger.printMessage("Client process: " + playerEntry.playerName + " is waiting for client to respond.");
                context = waitingResponseContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case WAITING_OTHER_PLAYER_RESPONSE:
                logger.printMessage("Client process: " + playerEntry.playerName + " is waiting for another player to respond.");
                context = waitingOtherPlayerResponseContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case READY_TO_PLAY:
                logger.printMessage("Client process: " + playerEntry.playerName + " is ready to play.");
                context = readyToPlayContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
            case MATCH:
                logger.printMessage("Client process: " + playerEntry.playerName + " is in a match.");
                context = matchContext(connectedSockfd, entryNumber, &heartbeat_expired);
                break;
        }
    }

    if (context == SOCKET_CONNECTED)
    {
        // If we got here after a wrong login, there won't be any entry, so we don't have to delete anything 
        // from the database. We have to tell the client that the login was incorrect
        logger.printMessage("TCP - Thread: This client did not login correctly.");
        logger.errorHandler.printErrorCode(status);
        respond(connectedSockfd, status);
    }
    else 
    {
        logger.printMessage("Client process: " + playerEntry.playerName + " logged out.");
        mainDatabase.clearEntry(entryNumber);
    }
        
            
    // Close the socket
    shutdown(connectedSockfd, SHUT_RDWR);
    close(connectedSockfd);
    // Previous ending the thread, we must free the slot.
    slots[slotNumber].clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

context_t lobbyContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    Logger logger;
    context_t context = LOBBY;
    T3PCommand t3pCommand;
    tcpcommand_t command;
    MainDatabaseEntry myEntry;
    list<string> availablePlayers;
    mainDatabase.setContext(entryNumber, context);
    myEntry = mainDatabase.getEntries()[entryNumber];
    string invitePlayer;

    while (context == LOBBY)
    {
        if (isHeartbeatExpired(heartbeat_expired, entryNumber))
            return DISCONNECT;

        if (mainDatabase.getEntries()[entryNumber].invitationStatus == PENDING)
        {
            context = WAITING_RESPONSE;
            break;
        }
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
            respond(connectedSockfd, status);
        if (t3pCommand.isNewCommand)
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    logger.printMessage("Client process: " + myEntry.playerName + " updated heartbeat");
                    break;
                case INVITE:
                    invitePlayer = t3pCommand.dataList.front();  
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
                    availablePlayers = mainDatabase.getAvailablePlayers(myEntry.playerName);
                    if (availablePlayers.empty())
                        respond(connectedSockfd, INFO_NO_PLAYERS_AVAILABLE);
                    else 
                    {
                        int random_number = rand() % availablePlayers.size();
                        for (int i=0; i < random_number; i++)
                            availablePlayers.pop_front();
                        invitePlayer = availablePlayers.front();
                        mainDatabase.setInvitation(mainDatabase.getEntryNumber(invitePlayer), myEntry.playerName);
                        respond(connectedSockfd, RESPONSE_OK);
                        mainDatabase.setContext(entryNumber, context);
                        context = WAITING_OTHER_PLAYER_RESPONSE;
                    }
                    break;
                case LOGOUT:
                    context = DISCONNECT;
                    mainDatabase.setContext(entryNumber, context);
                    respond(connectedSockfd, RESPONSE_OK);
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
        if (isHeartbeatExpired(heartbeat_expired, entryNumber))
            return DISCONNECT;

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
        if (t3pCommand.isNewCommand)
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
        if (isHeartbeatExpired(heartbeat_expired, entryNumber))
            return DISCONNECT;

        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        if (t3pCommand.isNewCommand)
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
                mainDatabase.setEntry(invitedPlayerEntryNumber, invitedPlayerEntry);
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
                mainDatabase.setEntry(invitedPlayerEntryNumber, invitedPlayerEntry);
                context = READY_TO_PLAY;
                sendInviteResponse(connectedSockfd, ACCEPT);
                break;
        }
    }
    return context;
}

context_t readyToPlayContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    context_t context = READY_TO_PLAY;
    T3PCommand t3pCommand;
    Logger logger;
    tcpcommand_t command;

    while (context == READY_TO_PLAY)
    {
        if (isHeartbeatExpired(heartbeat_expired, entryNumber))
            return DISCONNECT;

        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        if (t3pCommand.isNewCommand)
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    break;
            }
        }
        context = mainDatabase.getEntries()[entryNumber].context;
    }

    return context;
}

context_t matchContext(int connectedSockfd, int entryNumber, bool *heartbeat_expired)
{
    status_t status;
    context_t context = MATCH;
    Logger logger;
    T3PCommand t3pCommand;
    tcpcommand_t command;
    MainDatabaseEntry myEntry;
    MatchSlot playAs;
    bool myTurn;
    int matchEntryNumber;
    bool sendMessageTurnplay = true;
    bool sendMessageTurnwait = true;

    myEntry = mainDatabase.getEntry(entryNumber);

    while (!mainDatabase.getEntries()[entryNumber].matchEntryReady)
    {       
        // This shouldn't take so long, but just in case, read any heartbeat incomming  
        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        if (t3pCommand.isNewCommand)
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    break;
            }
        }
    }

    // Once the match entry is ready, we want to grab the entry number in the match database
    // and to know if I play as circle or cross.
    matchEntryNumber = mainDatabase.getEntries()[entryNumber].matchEntryNumber;
    if (matchDatabase[matchEntryNumber].circlePlayer == myEntry.playerName)
        playAs = CIRCLE;
    else    
        playAs = CROSS;

    while (context == MATCH)
    {
        if (matchDatabase[matchEntryNumber].plays == myEntry.playerName)
        {
            myTurn = true;
            if (sendMessageTurnplay)
            {
                if (sendTurnMessage(connectedSockfd, matchEntryNumber, myTurn) == STATUS_OK)
                    sendMessageTurnplay = false;
                sendMessageTurnwait = true;
            }
        }
        else
        {
            myTurn = false;
            if (sendMessageTurnwait)
            {
                if (sendTurnMessage(connectedSockfd, matchEntryNumber, myTurn) == STATUS_OK)
                    sendMessageTurnwait = false;
                sendMessageTurnplay = true;
            }
        }

        if (isHeartbeatExpired(heartbeat_expired, entryNumber))
        {
            // Before dying, tell the match thread that this client died.
            matchDatabase[matchEntryNumber].matchEndStatus = CONNECTION_LOST;
            if (playAs == CIRCLE)
            {
                matchDatabase[matchEntryNumber].circlePlayerLostConnection = true;
                matchDatabase[matchEntryNumber].circlePlayerEndConfirmation = true;
            }
            else
            {
                matchDatabase[matchEntryNumber].crossPlayerLostConnection = true;
                matchDatabase[matchEntryNumber].crossPlayerEndConfirmation = true;
            }
            matchDatabase[matchEntryNumber].matchEnded = true;
            return DISCONNECT;
        }
            

        if ((status = receiveMessage(connectedSockfd, &t3pCommand, context)) != STATUS_OK)
        {
            logger.errorHandler.printErrorCode(status);
            respond(connectedSockfd, status);
        }
        if (t3pCommand.isNewCommand)
        {
            command = TCPCommandTranslator[t3pCommand.command];
            switch(command)
            {
                case HEARTBEAT:
                    mainDatabase.udpateHeartbeat(entryNumber);
                    break;
                case GIVEUP:
                    if (playAs == CIRCLE)
                        matchDatabase[matchEntryNumber].circleGiveUp = true;
                    else
                        matchDatabase[matchEntryNumber].crossGiveUp = true;
                    respond(connectedSockfd, RESPONSE_OK);
                    break;
                case MARKSLOT:
                    // We first want to know if it's our turn
                    if (!myTurn)
                        respond(connectedSockfd, ERROR_NOT_TURN);
                    else 
                    {
                        int slotToMark = stoi(t3pCommand.dataList.front());
                        // We make this correction because in the RFC,
                        // the number in the MARKSLOT command is from 1 to 9,
                        // but in the actual vector is from 0 to 8.                         
                        if ((matchDatabase[matchEntryNumber].isSlotEmpty(slotToMark-1)) &&
                            (slotToMark > 0) && (slotToMark < 10))
                        {
                            respond(connectedSockfd, RESPONSE_OK);
                            matchDatabase[matchEntryNumber].markSlot(slotToMark-1, playAs);
                        }  
                        else
                            respond(connectedSockfd, ERROR_BAD_SLOT);
                    }
                    break;
                case OK:
                    break;
                case BAD:
                    break;
            }
        }
        context = mainDatabase.getEntries()[entryNumber].context;
    }

    sendMatchEnd(connectedSockfd, myEntry.playerName, matchEntryNumber);

    if (playAs == CIRCLE)
        matchDatabase[matchEntryNumber].circlePlayerEndConfirmation = true;
    else 
        matchDatabase[matchEntryNumber].crossPlayerEndConfirmation = true;

    mainDatabase.clearMatchEntryNumber(entryNumber);

    return context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

status_t receiveMessage(int sockfd, T3PCommand *t3pCommand, context_t context)
{
    status_t status;
    int read_bytes;
    int strip_bytes;
    int pos;
    string socket_message;
    char message[TCP_BUFFER_SIZE] = {0};
    (*t3pCommand).clear();
    
    if ((status = peek_tcp_buffer(sockfd, &read_bytes, &socket_message)) != STATUS_OK)
        return status;

    if (read_bytes > 0)
    {
        if ((pos = socket_message.find(" \r\n \r\n")) != string :: npos)
            strip_bytes = pos + strlen(" \r\n \r\n");
            
        recv(sockfd, message, strip_bytes, 0);
        if (parseMessage(string(message), t3pCommand) != STATUS_OK)
            return ERROR_BAD_REQUEST;
        if ((status = checkCommand(*t3pCommand, context)) != STATUS_OK)
            return status;
        t3pCommand->isNewCommand = true;
    }
    return STATUS_OK;
}

status_t peek_tcp_buffer(int sockfd, int *read_bytes, string *socket_message)
{
    char message[TCP_BUFFER_SIZE] = {0};
    *read_bytes = recv(sockfd, message, sizeof(message), MSG_PEEK);
    *socket_message = message;
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
            break;
        case READY_TO_PLAY:
            switch(command)
            {
                case HEARTBEAT:
                    break;
                default:
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }
            break;
        case MATCH:
            switch(command)
            {
                case HEARTBEAT:
                    break;
                case MARKSLOT:
                    break;
                case GIVEUP:
                    break;
                case OK:
                    break;
                case BAD:
                    break;
                default:
                    return ERROR_COMMAND_OUT_OF_CONTEXT;
            }
            break;
    }
    return STATUS_OK;
}

status_t respond(int sockfd, status_t response)
{
    string message = TCPResponseDictionary[response];
    return sendMessage(sockfd, message);
}

status_t sendInviteFrom(int sockfd, string invitingPlayer)
{
    string message = "INVITEFROM|";
    message += invitingPlayer;
    message += " \r\n \r\n";
    return sendMessage(sockfd, message);
}

status_t sendInvitationTimeout(int sockfd)
{
    string message = "INVITATIONTIMEOUT \r\n \r\n";
    return sendMessage(sockfd, message);
}

status_t sendInviteResponse(int sockfd, tcpcommand_t command)
{
    string message;
    switch(command)
    {
        case ACCEPT:
            message = "ACCEPT \r\n \r\n";
            break;
        case DECLINE:
            message = "DECLINE \r\n \r\n";
            break;
    }
    return sendMessage(sockfd, message);
}

status_t sendTurnMessage(int sockfd, int matchEntryNumber, bool myTurn)
{
    string message = "";
    string circleSlots = matchDatabase[matchEntryNumber].getFormattedSlots(CIRCLE);
    string crossSlots = matchDatabase[matchEntryNumber].getFormattedSlots(CROSS);

    if (myTurn)
        message = "TURNPLAY|";
    else 
        message = "TURNWAIT|";
    
    message += crossSlots + "|" + circleSlots + " \r\n \r\n";
    return sendMessage(sockfd, message);
}

status_t sendMatchEnd(int sockfd, string myName, int matchEntryNumber)
{
    string message = "MATCHEND|";
    switch (matchDatabase[matchEntryNumber].matchEndStatus)
    {
        case NORMAL:
            if (myName == matchDatabase[matchEntryNumber].winner)
                message += "YOUWIN \r\n \r\n";
            else if (matchDatabase[matchEntryNumber].winner == "")
                message += "DRAW \r\n \r\n";
            else
                message += "YOULOSE \r\n \r\n";
            break;
        case TIMEOUT:
            if (myName == matchDatabase[matchEntryNumber].winner)
                message += "TIMEOUTWIN \r\n \r\n";
            else
                message += "TIMEOUTLOSE \r\n \r\n";
            break;
        case CONNECTION_LOST:
            message += "CONNECTIONLOST \r\n \r\n";
            break;   
    }
        
    return sendMessage(sockfd, message);
}

status_t sendMessage(int sockfd, string message)
{
    const char *c_message;
    c_message = message.c_str();
    if (send(sockfd, c_message, strlen(c_message), 0) < 0)
        return ERROR_SENDING_MESSAGE;
    return STATUS_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bool isHeartbeatExpired(bool *heartbeat_expired, int entryNumber)
{
    Logger logger;
    if (mainDatabase.getEntries()[entryNumber].heartbeatExpired)
    {
        logger.printMessage("Client process: " + mainDatabase.getEntries()[entryNumber].playerName + " has heartbeat expired.");
        *heartbeat_expired = true;
        mainDatabase.setContext(entryNumber, DISCONNECT);
        return true;
    }
    return false;    
}