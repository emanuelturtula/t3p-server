#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include "../headers/udp_thread.h"
#include "../headers/config.h"

using namespace std;

/**
 * This is the thread for UDP messages. Here, we create the socket,
 * bind it to an interface IP, and respond DISCOVERBROADCASTS and
 * DISCOVER messages.
 * */
void processUDPMesages(int udpSocket)
{
    int sockfd;
    status_t status;
    Logger logger;
    char buffer[UDP_BUFFER_SIZE];
    
    // First, get the interfaces addresses using hints. We want an
    // IPv4 address, 

    logger.printMessage("UDP-THREAD: Creating listening socket");

    while(1)
    {
    }
}

