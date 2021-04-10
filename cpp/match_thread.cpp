#include "../headers/match_thread.h"
#include "../headers/tcp_thread.h"
#include "../headers/types.h"

extern MainDatabase mainDatabase;
extern vector<MatchEntry> matchDatabase;

void matchProcess(int firstPlayerEntryNumber, int secondPlayerEntryNumber)
{
    MatchEntry matchEntry;
    int slotNumber;
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
    for (slotNumber = 0; slotNumber++ < matchDatabase.size(); slotNumber++)
    {
        if (matchDatabase[slotNumber].free_slot)
            // As soon as we find a free slot, we reserve that space.
            matchDatabase[slotNumber].free_slot = false;
            break;
    }

    // We copy the players information in our slot. 
    matchDatabase[slotNumber].circlePlayer = matchEntry.circlePlayer;
    matchDatabase[slotNumber].crossPlayer = matchEntry.crossPlayer;
    // Circles always start, but we change who is the circle and who is the cross
    matchDatabase[slotNumber].plays = matchEntry.circlePlayer;

    // We have finished setting our match entry in the matchDatabase. Now let's indicate the players
    // where do they have to look in the match database.
    mainDatabase.setMatchEntryNumber(firstPlayerEntryNumber, slotNumber);
    mainDatabase.setMatchEntryNumber(secondPlayerEntryNumber, slotNumber);

    // The players should know that the match has started. Wait until the match is finished.
    while(!matchDatabase[slotNumber].matchEnded)
    {
        
    }

    // Now that the match has finished, free the slot
    MatchEntry newEntry;
    matchDatabase[slotNumber] = newEntry;
}