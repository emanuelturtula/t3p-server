#include "../headers/tcp_thread.h"
#include "../headers/types.h"

using namespace std;

void processClient(int connectedSockfd, int slotNumber)
{
    //The first thing to do, is check that the login is correct. Else,
    //we finish the thread.

    //Previous ending the thread, we must free the slot.
}

