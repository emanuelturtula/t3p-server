#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <map>
#include <list>
#include <regex>
#include "../headers/udp_thread.h"

using namespace std;

extern MainDatabase mainDatabase;
extern string serverIp;
extern map<string, string> config;

status_t receiveMessage(int sockfd, struct sockaddr_in *client, socklen_t *clientlen, T3PCommand *t3pCommand);
status_t parseUDPMessage(string message, T3PCommand *t3pCommand);
status_t checkCommand(T3PCommand t3pCommand);
status_t respond(int sockfd, struct sockaddr_in client, socklen_t clientlen, string message);


/**
 * This is the thread for UDP messages. Given a bound 
 * respond DISCOVERBROADCASTS and
 * DISCOVER messages.
 * */
void processUDPMesages(int udpSocket, const char *ip)
{
    int sockfd;
    status_t status;
    Logger logger;
    socklen_t clientlen;
    struct sockaddr_in client = {0};
    T3PCommand t3pCommand;
    string message;

    int broadcastEnable = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    while(1)
    {
        t3pCommand.clear();
        message = "";
        memset(&client, 0, sizeof(client));
        clientlen = sizeof(struct sockaddr_in);
        if ((status = receiveMessage(udpSocket, &client, &clientlen, &t3pCommand)) != STATUS_OK)
        {
            logger.debugMessage("UDP Thread - Error receiving message");
            logger.printErrorMessage(status);
            switch(status)
            {
                case ERROR_BAD_REQUEST:
                    message = "400|Bad Request";
                    message += " \r\n \r\n";
                    respond(udpSocket, client, clientlen, message);
                    break;
                // Don't know if we have more errors to process
            }
        }
        else
        {
            if (t3pCommand.command == "DISCOVERBROADCAST")
            {
                logger.debugMessage("UDP Thread - Received DISCOVERBROADCAST");
                message = "200|OK|";
                message += string(ip);
                message += " \r\n \r\n";
                respond(udpSocket, client, clientlen, message);
            }
            else if (t3pCommand.command == "DISCOVER")
            {
                // if (t3pCommand.dataList.front() == string(ip))
                // {
                logger.debugMessage("UDP Thread - Received DISCOVER");
                list<string> availablePlayers = mainDatabase.getAvailablePlayers();
                list<string> occupiedPlayers = mainDatabase.getOccupiedPlayers();
                message = "200|OK|";
                for(auto const& player : availablePlayers)
                {
                    message += player;
                    message += "|";
                }
                if (!availablePlayers.empty())
                    message.erase(prev(message.end()));
                message += " \r\n";
                for(auto const& player : occupiedPlayers)
                {
                    message += player;
                    message += "|";
                }
                if (!occupiedPlayers.empty())
                    message.erase(prev(message.end()));
                message += " \r\n \r\n";
                respond(udpSocket, client, clientlen, message);
                // }
            }
        }
    }
}

status_t receiveMessage(int sockfd, struct sockaddr_in *client, socklen_t *clientlen, T3PCommand *t3pCommand)
{
    status_t status;
    char message[UDP_BUFFER_SIZE] = {0};
    if (recvfrom(sockfd, message, sizeof(message), 0, (struct sockaddr *)client, clientlen) < 0)
        return ERROR_SERVER_ERROR;
    else
    {
        if (parseUDPMessage(string(message), t3pCommand) != STATUS_OK)
            return ERROR_BAD_REQUEST;
        if ((status = checkCommand(*t3pCommand)) != STATUS_OK)
            return status;
    }
    return STATUS_OK;   
}

status_t parseUDPMessage(string message, T3PCommand *t3pCommand)
{
    size_t pos;
    if ((pos = message.rfind(" \r\n \r\n")) == string::npos)
        return ERROR_BAD_REQUEST;
    
    // Strip last \r\n
    message.erase(pos+3);

    // If message contains "|", grab the first part as a command
    if ((pos = message.find("|")) != string::npos)
    {
        t3pCommand->command = message.substr(0, pos);
        message.erase(0, pos+1);
        while ((pos = message.find(" \r\n")) != string::npos)
        {
            t3pCommand->dataList.push_back(message.substr(0, pos));
            message.erase(0, pos+3);
        }
    }
    // else, the message should only contain the command
    else 
    {
        //I must have it because we checked at the beginning
        pos = message.find(" \r\n");
        t3pCommand->command = message.substr(0, pos);
    }
        
    return STATUS_OK;
}

status_t checkCommand(T3PCommand t3pCommand)
{
    regex ip_checker("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    if ((t3pCommand.command != "DISCOVER") && (t3pCommand.command != "DISCOVERBROADCAST"))
        return ERROR_BAD_REQUEST;
    if (t3pCommand.command == "DISCOVER")
    {
        // if (!regex_match(t3pCommand.dataList.front(), ip_checker))
        //     return ERROR_BAD_REQUEST;
        // if (t3pCommand.dataList.front() != serverIp)
        //     return ERROR_BAD_REQUEST;
    }
    return STATUS_OK;
}

status_t respond(int sockfd, struct sockaddr_in client, socklen_t clientlen, string message)
{
    const char *c_message = message.c_str();
    if (sendto(sockfd, c_message, strlen(c_message), 0, (struct sockaddr *)&client, clientlen) < 0)
        return ERROR_SENDING_MESSAGE;
    return STATUS_OK;
}