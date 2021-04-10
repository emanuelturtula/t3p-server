#include "../headers/referee_thread.h"
#include "../headers/match_thread.h" 
#include "../headers/types.h"
#include <thread>
#include <unistd.h>

using namespace std;

extern MainDatabase mainDatabase;

/**
 * This thread is constantly reading the database looking for 2 players to be ready to play with each other.\
 * When it finds them, it creates another thread for the match and puts both players in that context.
 * */
void refereeProcess()
{
    Logger logger;
    MainDatabaseEntry firstPlayerEntry, secondPlayerEntry;
    bool match_found;
    while(1)
    {
        for (int firstPlayerEntryNumber = 0; firstPlayerEntryNumber < mainDatabase.getEntries().size(); firstPlayerEntryNumber++)
        {
            firstPlayerEntry = mainDatabase.getEntry(firstPlayerEntryNumber);
            // We find the first player with context in READY_TO_PLAY
            if (firstPlayerEntry.context == READY_TO_PLAY)
            {
                match_found = false;
                // There should be another one in the rest of the entries
                for (int secondPlayerEntryNumber = firstPlayerEntryNumber; secondPlayerEntryNumber < mainDatabase.getEntries().size(); secondPlayerEntryNumber++)
                {
                    secondPlayerEntry = mainDatabase.getEntry(secondPlayerEntryNumber);
                    // If we find another player in READY_TO_PLAY context and is ready to play with the first player...
                    if ((secondPlayerEntry.context == READY_TO_PLAY) && (secondPlayerEntry.readyToPlayWith == firstPlayerEntry.playerName))
                    {
                        // We double check just in case
                        if (firstPlayerEntry.readyToPlayWith == secondPlayerEntry.playerName)
                        {
                            // These two have to be in a match. We then put them both in a match context
                            logger.printMessage("Referee: " + firstPlayerEntry.playerName + " will be in a match with " + secondPlayerEntry.playerName);
                            mainDatabase.setContext(firstPlayerEntryNumber, MATCH);
                            mainDatabase.setContext(secondPlayerEntryNumber, MATCH);
                            // We create a thread for them
                            thread match(matchProcess, firstPlayerEntryNumber, secondPlayerEntryNumber);
                            match.detach();
                            match_found = true;
                        }
                    }
                    // If we have found a match, then we continue to look another first player with context in READY_TO_PLAY
                    if (match_found)
                        break;
                }
            }       
        }
        sleep(1);
    }
}

// grizz1 READY_TO_PLAY
// edturtu READY_TO_PLAY