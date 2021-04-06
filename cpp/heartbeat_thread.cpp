#include <ctime>
#include <unistd.h>
#include "../headers/config.h"
#include "../headers/types.h"

using namespace std;

extern MainDatabase mainDatabase;

void heartbeatChecker()
{
    int entryNumber;
    while (1)
    {
        for (auto const& entry : mainDatabase.getEntries())
        {
            if (entry.slotNumber != -1)
            {
                if (time(NULL) - entry.lastHeartbeat > HEARTBEAT_SECONDS_TIMEOUT)
                {
                    entryNumber = mainDatabase.getEntryNumber(entry.playerName);
                    mainDatabase.setContext(entryNumber, HEARTBEAT_EXPIRED);
                    // This should be enough to tell the corresponding tcp thread that it 
                    // must be closed
                }     
            }
        }
        // Sleep 1 second
        sleep(1);
    }
}