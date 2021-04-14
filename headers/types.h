#pragma once

#include <string>
#include <list>
#include <thread>
#include <ctime>
#include <vector>

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

using namespace std;

enum status_t {
    // Internal server Errors:
    STATUS_OK = 0,
    ERROR_NULL_POINTER,
    ERROR_SOCKET_CREATION,
    ERROR_SOCKET_CONFIGURATION,
    ERROR_SOCKET_LISTENING,
    ERROR_SOCKET_READING,
    ERROR_SOCKET_BINDING,
    ERROR_SENDING_MESSAGE,
    ERROR_RECEIVING_MESSAGE,


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
    ERROR_SERVER_ERROR = 500,
};

enum context_t {
    SOCKET_CONNECTED,
    LOBBY,
    WAITING_RESPONSE,
    WAITING_OTHER_PLAYER_RESPONSE,
    READY_TO_PLAY,
    MATCH,
    DISCONNECT,
    DISCONNECT_HEARTBEAT,
    DISCONNECT_ERROR
};

enum invitation_status_t {
    REJECTED,
    ACCEPTED,
    PENDING,
    NONE
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
        void printErrorMessage(status_t);
        void printErrorMessage(string message, status_t status);
        void printMessage(string);
        void printMessage(string message, string color);
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

class T3PCommand {
    public:
        T3PCommand();
        void clear();
        string command;
        list<string> dataList;
        bool isNewCommand = false;
};

class Slot {
    public:
        Slot();
        void clear();
        bool available;
};

class MainDatabaseEntry {
    public:
        MainDatabaseEntry();
        // Use slotNumber = -1 to indicate the entry space is not being used.
        int slotNumber = -1;
        // This will be used by the referee to send invitations  
        int connectedSockfd;
        string playerName;
        invitation_status_t invitationStatus = NONE;
        string invitingPlayerName = "";
        context_t context;
        time_t lastHeartbeat;
        string readyToPlayWith = "";
        bool heartbeatExpired = false;
        bool matchEntryReady = false;
        int matchEntryNumber = -1;
};

class MainDatabase {
    vector<MainDatabaseEntry> entries;
    public:
        MainDatabase();
        void resizeEntries(int size);
        void clearEntry(int entryNumber);
        int getAvailableEntry();
        int getSlotNumber(int entryNumber);
        context_t getContext(int entryNumber);
        MainDatabaseEntry getEntry(int entryNumber);
        int getEntryNumber(string playerName);
        int getInvitedPlayerEntryNumber(string playerName);
        vector<MainDatabaseEntry> getEntries();
        list<string> getPlayersOnline();
        list<string> getAvailablePlayers();
        list<string> getAvailablePlayers(string excludePlayer);
        list<string> getOccupiedPlayers();
        void setEntry(int entryNumber, MainDatabaseEntry entry); 
        void setContext(int entryNumber, context_t context);
        void setInvitation(int entryNumber, string invitingPlayer);
        void setInvitation(int entryNumber, invitation_status_t invitationStatus);
        void setMatchEntryNumber(int entryNumber, int matchEntryNumber);
        void clearMatchEntryNumber(int entryNumber);
        void setHeartbeatExpired(int entryNumber);
        void udpateHeartbeat(int entryNumber);
};

enum MatchSlot {
    CIRCLE,
    CROSS,
    EMPTY
};

enum MatchEndStatus {
    CONNECTION_LOST,
    TIMEOUT,
    NORMAL
};

class MatchEntry {
    vector<MatchSlot> slots;
    public:
        bool free_entry = true;
        string circlePlayer = "";
        string crossPlayer = "";
        string winner = "";
        string plays;
        bool matchEnded = false;
        MatchEndStatus matchEndStatus = NORMAL;
        bool circleGiveUp = false;
        bool crossGiveUp = false;
        bool circlePlayerEndConfirmation = false;
        bool crossPlayerEndConfirmation = false;
        bool circlePlayerLostConnection = false;
        bool crossPlayerLostConnection = false;
        MatchEntry();
        void clearEntry();
        void clearSlots();
        bool isSlotEmpty(int slotNumber);
        void markSlot(int slotNumber, MatchSlot slotType);
        vector<MatchSlot> getSlots();
        string getFormattedSlots(MatchSlot slotType);
};

int getAvailableMatchEntry();

