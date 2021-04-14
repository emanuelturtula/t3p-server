#include <ctime>
#include <unistd.h>
#include <map>
#include <string>
#include "../headers/types.h"

using namespace std;

extern MainDatabase mainDatabase;
extern map<string, int> config;

void heartbeatChecker()
{
    int entryNumber;
    Logger logger;
    string message;
    int heartbeatTimeout = config["HEARTBEAT_TIMEOUT"];
    while (1)
    {
        for (auto const& entry : mainDatabase.getEntries())
        {
            if (entry.slotNumber != -1)
            {
                message = "Heartbeat checker - Checking slot: ";
                message += to_string(entry.slotNumber);
                logger.debugMessage(message);
                if (time(NULL) - entry.lastHeartbeat > heartbeatTimeout)
                {
                    message = "Heartbeat expired for player: ";
                    message += entry.playerName;
                    logger.debugMessage(message);
                    entryNumber = mainDatabase.getEntryNumber(entry.playerName);
                    mainDatabase.setHeartbeatExpired(entryNumber);
                    // This should be enough to tell the corresponding tcp thread that it 
                    // must be closed
                }     
            }
        }
        // Sleep 1 second
        sleep(1);
    }
}