#include <iostream>
#include <mutex>
#include "../headers/types.h"
#include "../headers/config.h"


using namespace std;

mutex m_stdout, m_stderr, m_database;

/**
 * Methods for Logger
 * */
// Should include writing all in a log file in the future. In the meantime, it writes status of program
// in the terminal
Logger :: Logger()
{

}

void Logger :: printMessage(status_t status)
{
    this->printer.writeStderr("Got status ID = " + to_string(status));
}

void Logger :: printMessage(string message)
{
    this->printer.writeStderr(message);
}

void Logger :: printMessage(string message, status_t status)
{
    this->printMessage(status);
    this->printMessage(message);
}

void Logger :: debugMessage(string message)
{
    #if defined(DEBUG_MESSAGES)
        this->printMessage(message);
    #endif
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
    this->entries.resize(MAX_PLAYERS_ONLINE);
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

int MainDatabase :: getEntryNumber(string playerName)
{
    for (int entryNumber = 0; entryNumber < this->entries.size(); entryNumber++)
    {
        if (this->entries[entryNumber].playerName == playerName)
            return entryNumber;
    }
    return -1;
}

bool MainDatabase :: getInvitationPending(int entryNumber)
{
    return this->entries[entryNumber].invitationPending;
}

time_t MainDatabase :: getInvitationTime(int entryNumber)
{
    return this->entries[entryNumber].invitationTime;
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
            (! this->entries[i].invitationPending))
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
    this->entries[entryNumber].invitationPending = true;
    this->entries[entryNumber].invitingPlayerName = invitingPlayer;
}

void MainDatabase :: udpateHeartbeat(int entryNumber)
{
    time(&(this->entries[entryNumber].lastHeartbeat));
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
}

