#include "../headers/match_thread.h"
#include "../headers/tcp_thread.h"
#include "../headers/types.h"
#include <map>
#include <ctime>

extern MainDatabase mainDatabase;
extern vector<MatchEntry> matchDatabase;
extern map<string, int> config;

MatchSlot checkBoard(vector<MatchSlot> boardSlots, bool *draw);

void matchProcess(int firstPlayerEntryNumber, int secondPlayerEntryNumber)
{
    MatchEntry matchEntry;
    int matchEntryNumber;
    bool draw = false;
    MatchSlot slotTypeWinner;
    Logger logger;
    int markslotTimeout = config["MARKSLOT_TIMEOUT"];
    // Here we prepare the match entry and write the players' database entries 
    // so they can see which match entry correspond to them.
    // When we have two players logged in, they won't switch their places in the database,
    // and that will cause that, if they play consecutive matches, the first player will
    // always be the first one found in the database.
    // To prevent that, we would like to make some kind of random function to select who is going to start.
    // So we generate a random number between 0 an 9

    int randomNumber = rand() % 10; 
    if (randomNumber < 5)
    {
        matchEntry.circlePlayer = mainDatabase.getEntry(firstPlayerEntryNumber).playerName;
        matchEntry.crossPlayer = mainDatabase.getEntry(secondPlayerEntryNumber).playerName;
    }
    else 
    {
        matchEntry.circlePlayer = mainDatabase.getEntry(secondPlayerEntryNumber).playerName;
        matchEntry.crossPlayer = mainDatabase.getEntry(firstPlayerEntryNumber).playerName;
    }

    // Now we have to find a free slot in the matchDatabase. It won't ever happen that we don't find an
    // available slot, because that would mean that every player is in a match, so new invitations
    // can't exist.
    matchEntryNumber = getAvailableMatchEntry();
    
    // We copy the players information in our slot. 
    matchDatabase[matchEntryNumber].circlePlayer = matchEntry.circlePlayer;
    matchDatabase[matchEntryNumber].crossPlayer = matchEntry.crossPlayer;
    // Circles always start, but we change who is the circle and who is the cross
    matchDatabase[matchEntryNumber].plays = matchEntry.circlePlayer;

    // We have finished setting our match entry in the matchDatabase. Now let's indicate the players
    // where do they have to look in the match database.
    mainDatabase.setMatchEntryNumber(firstPlayerEntryNumber, matchEntryNumber);
    mainDatabase.setMatchEntryNumber(secondPlayerEntryNumber, matchEntryNumber);

    // The players should know that the match has started. Wait until the match has finished.
    vector<MatchSlot> lastSlotsStatus = matchDatabase[matchEntryNumber].getSlots();
    time_t lastSlotChangeTime;
    time(&lastSlotChangeTime);
    while(!matchDatabase[matchEntryNumber].matchEnded)
    {
        if (matchDatabase[matchEntryNumber].circleGiveUp)
        {
            matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].crossPlayer;
            matchDatabase[matchEntryNumber].matchEnded = true;
            break;
        }
        if (matchDatabase[matchEntryNumber].crossGiveUp)
        {
            matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].circlePlayer;
            matchDatabase[matchEntryNumber].matchEnded = true;
            break;
        }     

        #ifdef DEBUG_MESSAGES
            if (matchDatabase[matchEntryNumber].plays == matchDatabase[matchEntryNumber].circlePlayer)
                logger.debugMessage("Player " + matchDatabase[matchEntryNumber].circlePlayer + " has " + to_string(MARKSLOT_SECONDS_TIMEOUT - (time(NULL) - lastSlotChangeTime)) + " seconds to play.");
            else
                logger.debugMessage("Player " + matchDatabase[matchEntryNumber].crossPlayer + " has " + to_string(MARKSLOT_SECONDS_TIMEOUT - (time(NULL) - lastSlotChangeTime)) + " seconds to play.");
        #endif

        if (time(NULL) - lastSlotChangeTime > markslotTimeout)
        {
            // If a player doesn't interact in a while, we make him lose 
            // because of timeout
            // If it was the CIRCLEs turn, he lost
            if (matchDatabase[matchEntryNumber].plays == matchDatabase[matchEntryNumber].circlePlayer)
                matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].crossPlayer;
            else
                matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].circlePlayer;
            // We indicate that the match ended because of timeout
            matchDatabase[matchEntryNumber].matchEndStatus = TIMEOUT;
            matchDatabase[matchEntryNumber].matchEnded = true;
            break;
        }

        if (lastSlotsStatus != matchDatabase[matchEntryNumber].getSlots())
        {
            //  If there's an update in the slots, check first if there is any winner or draw
            time(&lastSlotChangeTime);
            lastSlotsStatus = matchDatabase[matchEntryNumber].getSlots();            
            slotTypeWinner = checkBoard(lastSlotsStatus, &draw);

            if (slotTypeWinner != EMPTY)
            {
                if (slotTypeWinner == CIRCLE)
                    matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].circlePlayer;
                else
                    matchDatabase[matchEntryNumber].winner = matchDatabase[matchEntryNumber].crossPlayer;
                matchDatabase[matchEntryNumber].matchEnded = true;
            }
            else
            {
                if (draw)
                    matchDatabase[matchEntryNumber].matchEnded = true;
            }
            //  Then update the turn if the game is still being played.
            if (!matchDatabase[matchEntryNumber].matchEnded)
            {
                if (matchDatabase[matchEntryNumber].plays == matchDatabase[matchEntryNumber].circlePlayer)
                    matchDatabase[matchEntryNumber].plays = matchDatabase[matchEntryNumber].crossPlayer;
                else
                    matchDatabase[matchEntryNumber].plays = matchDatabase[matchEntryNumber].circlePlayer; 
            }
        }
    }
    
    
    if (matchDatabase[matchEntryNumber].matchEndStatus != CONNECTION_LOST)
    {
        mainDatabase.setContext(firstPlayerEntryNumber, LOBBY);
        mainDatabase.setContext(secondPlayerEntryNumber, LOBBY);
    }
    else
    {
        // When the connection is lost, it can happen to both players at the same time (weird though).
        // To prevent sending to the lobby someone who already has lost connection, 
        // we have to check who lost connection.
        if (matchDatabase[matchEntryNumber].circlePlayerLostConnection == false)
            mainDatabase.setContext(mainDatabase.getEntryNumber(matchDatabase[matchEntryNumber].circlePlayer), LOBBY); 
            
        if (matchDatabase[matchEntryNumber].crossPlayerLostConnection == false)
            mainDatabase.setContext(mainDatabase.getEntryNumber(matchDatabase[matchEntryNumber].crossPlayer), LOBBY);
    }
    
    // Wait until the clients' threads mark the data read
    while((!matchDatabase[matchEntryNumber].circlePlayerEndConfirmation) || (!matchDatabase[matchEntryNumber].crossPlayerEndConfirmation));

    // Now that the match has finished, free the slot
    matchDatabase[matchEntryNumber].clearEntry();
}

MatchSlot checkBoard(vector<MatchSlot> boardSlots, bool *draw)
{
    int i;
    // Check horizontals first
    // Horizontals are 0,1,2; 3,4,5; 6,7,8
    for (i=0; i < 7; i+=3)
    {
        if ((boardSlots[i] == boardSlots[i+1]) && 
            (boardSlots[i] == boardSlots[i+2]) &&
            (boardSlots[i] != EMPTY))
            return boardSlots[i];            
    }

    // Check verticals
    // Verticals are 0,3,6; 1,4,7; 2,5,8
    for (i=0; i<3; i+=1)
    {
        if ((boardSlots[i] == boardSlots[i+3]) &&
            (boardSlots[i] == boardSlots[i+6]) && 
            (boardSlots[i] != EMPTY))
            return boardSlots[i];
    }

    // Check diagonals
    // Diagonals are 0,4,8; 2,4,6
    if ((boardSlots[0] == boardSlots[4]) &&
        (boardSlots[0] == boardSlots[8]) &&
        (boardSlots[4] != EMPTY))
        return boardSlots[4];
    
    if ((boardSlots[2] == boardSlots[4]) &&
        (boardSlots[2] == boardSlots[6]) &&
        (boardSlots[4] != EMPTY))
        return boardSlots[4];


    // If we got here, is because we couldn't find any winner. 
    // Let's check if the board is full to see if it's a draw.
    for (auto const& slot : boardSlots)
    {
        // Finding an EMPTY slot is enough to say it is not a draw
        if (slot == EMPTY)
        {
            *draw = false;
            return EMPTY;
        }   
    }

    // If we got here, it means the board is full and there isn't
    // any winner
    *draw = true;
    return EMPTY;
}
