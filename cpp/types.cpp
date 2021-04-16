#include <iostream>
#include <mutex>
#include <map>
#include "../headers/types.h"


using namespace std;

mutex m_stdout, m_stderr, m_database, m_match;

extern map<string, int> config;

map<status_t, string> T3PErrorCodes = {
    {ERROR_BAD_REQUEST, "400|Bad Request"},
    {ERROR_INCORRECT_NAME, "401|Incorrect Name"},
    {ERROR_NAME_TAKEN, "402|Name Taken"},
    {ERROR_PLAYER_NOT_FOUND, "403|Player not Found"},
    {ERROR_PLAYER_OCCUPIED, "404|Player Occupied"},
    {ERROR_BAD_SLOT, "405|Bad Slot"},
    {ERROR_NOT_TURN, "406|Not Turn"},
    {ERROR_INVALID_COMMAND, "407|Invalid command"},
    {ERROR_COMMAND_OUT_OF_CONTEXT, "408|Command out of context"},
    {ERROR_CONNECTION_LOST, "409|Connection Lost"},
    {ERROR_SERVER_ERROR, "500|Server Error"}
};

/**
 * Methods for Logger
 * */
// Should include writing all in a log file in the future. In the meantime, it writes status of program
// in the terminal
Logger :: Logger()
{

}

void Logger :: printErrorMessage(status_t status)
{
    this->printer.writeStderr("Got status ID = " + to_string(status) + ": " + T3PErrorCodes[status]);
}

void Logger :: printErrorMessage(string message, status_t status)
{
    this->printErrorMessage(status);
    this->printer.writeStderr(message);
}

void Logger :: printMessage(string message)
{
    this->printer.writeStdout(message);
}

void Logger :: printMessage(string message, string color)
{
    string msg = color + message + RESET;
    this->printer.writeStdout(msg);
}

void Logger :: debugMessage(string message)
{
    if (config["DEBUG_MESSAGES"])
        this->printMessage(message);
}


/**
 * Methods for ErrorHandler
 * */
ErrorHandler :: ErrorHandler()
{

}

void ErrorHandler :: printErrorCode(status_t status)
{
    Printer printerObj;
    printerObj.writeStderr("Error ID = " + to_string(status));
}

/**
 * Methods for Printer
 * */
Printer :: Printer()
{

}

// Writing in stdout or stderr might be used simultaneously by several threads, so we want
// to ensure that the streams are protected against that.
void Printer :: writeStdout(string message)
{
    lock_guard<mutex> lock(m_stdout);
    cout << message << endl;
}

void Printer :: writeStderr(string message)
{
    lock_guard<mutex> lock(m_stderr);
    cout << message << endl;
}

/**
 * Methods for Slot
 * */
Slot :: Slot()
{
    this->available = true; 
}

void Slot :: clear()
{
    this->available = true;
}


/**
 * Methods for MainDatabaseEntry
 * */
MainDatabaseEntry :: MainDatabaseEntry()
{

}

/**
 * Methods for MainDatabase
 * */
MainDatabase :: MainDatabase()
{

}

void MainDatabase :: resizeEntries(int size)
{
    this->entries.resize(size);
}

void MainDatabase :: clearEntry(int entryNumber)
{
    MainDatabaseEntry entry;
    this->entries[entryNumber] = entry;
}

// We have to do it this way to ensure only one thread reads the database,
// avoiding two or more threads getting the same entry number.
int MainDatabase :: getAvailableEntry()
{
    lock_guard<mutex> lock(m_database);
    for (int i = 0; i < this->entries.size(); i++)
    {
        if (this->entries[i].slotNumber == -1)
            return i;
    }  
    // Actually, we shouldn't ever get here if everything works fine. 
    // We have 50 slots, and 50 entries maximum. The client should never
    // try to look for a free entry if there were no slots available.  
    // But just in case, we return -1 as 'no available free entry'
    return -1;
}

int MainDatabase :: getSlotNumber(int entryNumber)
{
    return this->entries[entryNumber].slotNumber;
}

context_t MainDatabase :: getContext(int entryNumber)
{
    lock_guard<mutex> lock(m_database);
    return this->entries[entryNumber].context;
}

int MainDatabase :: getEntryNumber(string playerName)
{
    for (int entryNumber = 0; entryNumber < this->entries.size(); entryNumber++)
    {
        if (this->entries[entryNumber].playerName == playerName)
            return entryNumber;
    }
    return -1;
}

vector<MainDatabaseEntry> MainDatabase :: getEntries()
{
    lock_guard<mutex> lock(m_database);
    return this->entries;
}

list<string> MainDatabase :: getPlayersOnline()
{   
    lock_guard<mutex> lock(m_database);
    list<string> playersOnline;
    for (int i = 0; i < this->entries.size(); i++)
    {
        if (! this->entries[i].playerName.empty())
            playersOnline.push_back(this->entries[i].playerName);
    }    
    return playersOnline;
}

list<string> MainDatabase :: getAvailablePlayers()
{   
    lock_guard<mutex> lock(m_database);
    list<string> availablePlayers;
    for (int i = 0; i < this->entries.size(); i++)
    {
        //A player is available when the name is not empty, the context is lobby and it has no invitation pending.
        if ((! this->entries[i].playerName.empty()) && 
            (this->entries[i].context == LOBBY) &&
            (this->entries[i].invitationStatus == NONE))
            availablePlayers.push_back(this->entries[i].playerName);
    }    
    return availablePlayers;
}

list<string> MainDatabase :: getAvailablePlayers(string excludePlayer)
{   
    lock_guard<mutex> lock(m_database);
    list<string> availablePlayers;
    for (int i = 0; i < this->entries.size(); i++)
    {
        //A player is available when the name is not empty, the context is lobby and it has no invitation pending.
        if ((! this->entries[i].playerName.empty()) && 
            (this->entries[i].context == LOBBY) &&
            (this->entries[i].invitationStatus == NONE) &&
            (this->entries[i].playerName != excludePlayer))
            availablePlayers.push_back(this->entries[i].playerName);
    }    
    return availablePlayers;
}

list<string> MainDatabase :: getOccupiedPlayers()
{   
    lock_guard<mutex> lock(m_database);
    list<string> occupiedPlayers;
    for (int i = 0; i < this->entries.size(); i++)
    {
        if ((! this->entries[i].playerName.empty()) && (this->entries[i].context != LOBBY))
            occupiedPlayers.push_back(this->entries[i].playerName);
    }    
    return occupiedPlayers;
}

int MainDatabase :: getInvitedPlayerEntryNumber(string playerName)
{
    lock_guard<mutex> lock(m_database);
    for (int i = 0; i < this->entries.size(); i++)
    {
        if (this->entries[i].invitingPlayerName == playerName)
            return i;
    }
    return -1;
}
// It is not necessary to place a mutex in this function, because we know
// that when we call it, there won't be two or more threads with the 
// same entry number, so writing is actually secured (if used wisely).
void MainDatabase :: setEntry(int entryNumber, MainDatabaseEntry entry)
{
    this->entries[entryNumber] = entry;
}

void MainDatabase :: setContext(int entryNumber, context_t context)
{
    this->entries[entryNumber].context = context;
}

void MainDatabase :: setInvitation(int entryNumber, string invitingPlayer)
{
    this->entries[entryNumber].invitationStatus = PENDING;
    this->entries[entryNumber].invitingPlayerName = invitingPlayer;
}

void MainDatabase :: setInvitation(int entryNumber, invitation_status_t invitationStatus)
{
    this->entries[entryNumber].invitationStatus = invitationStatus;
}

void MainDatabase :: setHeartbeatExpired(int entryNumber)
{
    this->entries[entryNumber].heartbeatExpired = true;
}

void MainDatabase :: udpateHeartbeat(int entryNumber)
{
    time(&(this->entries[entryNumber].lastHeartbeat));
}

MainDatabaseEntry MainDatabase :: getEntry(int entryNumber)
{
    return this->entries[entryNumber];
}

void MainDatabase :: setMatchEntryNumber(int entryNumber, int matchEntryNumber)
{
    this->entries[entryNumber].matchEntryNumber = matchEntryNumber;
    this->entries[entryNumber].matchEntryReady = true;
}

void MainDatabase :: clearMatchEntryNumber(int entryNumber)
{
    this->entries[entryNumber].matchEntryNumber = -1;
    this->entries[entryNumber].matchEntryReady = false;
}

/**
 * Methods for T3PCommand
 * */
T3PCommand :: T3PCommand()
{
    this->command = "";
    this->dataList.clear();
}

void T3PCommand :: clear()
{
    this->command = "";
    this->dataList.clear();
    this->isNewCommand = false;
}

/**
 * Methods for MatchEntry
 * */

MatchEntry :: MatchEntry()
{
    int i;
    this->slots.resize(9);
    this->clearSlots();
}

void MatchEntry :: clearEntry()
{
    MatchEntry entry;
    this->clearSlots();
    this->circlePlayer = "";
    this->crossPlayer = "";
    this->winner = "";
    this->plays = "";
    this->matchEnded = false;
    this->matchEndStatus = NORMAL;
    this->circleGiveUp = false;
    this->crossGiveUp = false;
    this->circlePlayerEndConfirmation = false;
    this->crossPlayerEndConfirmation = false;
    this->circlePlayerLostConnection = false;
    this->crossPlayerLostConnection = false;
    this->free_entry = true;
}

void MatchEntry :: clearSlots()
{
    int i;
    for (i = 0; i < this->slots.size(); i++)
    {
        this->slots[i] = EMPTY;
    }
}

void MatchEntry :: markSlot(int slotNumber, MatchSlot slotType)
{
    this->slots[slotNumber] = slotType;
}

vector<MatchSlot> MatchEntry :: getSlots()
{
    return this->slots;
}

string MatchEntry :: getFormattedSlots(MatchSlot slotType)
{
    string formattedSlots = "";
    int i = 1;
    for (auto const& slot : this->slots)
    {
        if (slot == slotType)
            formattedSlots += to_string(i);
        i++;
    }
    return formattedSlots;
}

bool MatchEntry :: isSlotEmpty(int slotNumber)
{
    if (this->slots[slotNumber] == EMPTY)
        return true;
    return false;
}

int getAvailableMatchEntry()
{
    extern vector<MatchEntry> matchDatabase;
    int matchEntryNumber;
    lock_guard<mutex> lock(m_match);
    for (matchEntryNumber = 0; matchEntryNumber < matchDatabase.size(); matchEntryNumber++)
    {
        if (matchDatabase[matchEntryNumber].free_entry)
        {
            // As soon as we find a free slot, we reserve that space.
            matchDatabase[matchEntryNumber].free_entry = false;
            break;
        }
    }
    return matchEntryNumber;
}