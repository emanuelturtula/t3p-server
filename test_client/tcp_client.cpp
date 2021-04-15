#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <poll.h>


using namespace std;

mutex m;

int poll_event(int connectedSockfd, string *stdin_message, string *socket_message);

int main(int argc, char *argv[])
{
    int sockfd;
    int bytes;
    string stdin_message;
    string socket_message;
    const char *c_message;
    struct sockaddr_in server = {0};

    if (argc != 2)
    {
        cerr << "Missing arguments" << endl;
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];

    server.sin_family = AF_INET;
    server.sin_port = htons(2000);
    server.sin_addr.s_addr = inet_addr(ip);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) != 0)
        return EXIT_FAILURE;
    

    while (1)
    {
        if (poll_event(sockfd, &stdin_message, &socket_message) == -1)
        {
            cerr << "Error polling" << endl;
            break;
        }
        if (stdin_message != "")
        {
            if (stdin_message == "exit")
            {
                cout << "Exiting..." << endl;
                close(sockfd);
                return EXIT_SUCCESS;
            }
            stdin_message += " \r\n \r\n";
            c_message = stdin_message.c_str();
            if (send(sockfd, c_message, strlen(c_message), 0) < 0)
            {
                cerr << "Error sending message" << endl;
                close(sockfd);
                return EXIT_SUCCESS;
            }
        }
        if (socket_message != "")
        {
            socket_message.erase(socket_message.rfind(" \r\n \r\n"));
            cout << "\033[1m\033[33m" << socket_message << "\033[0m" << endl;
        }
            
    }

    close(sockfd);
    return EXIT_SUCCESS;
}


int poll_event(int connectedSockfd, string *stdin_message, string *socket_message)
{   
    struct pollfd pfds[2]; // We monitor sockfd and stdin
    char buffer[1024];

    *stdin_message = "";
    *socket_message = "";

    pfds[0].fd = 0;        // Stdin input
    pfds[0].events = POLLIN;    // Tell me when ready to read
    
    pfds[1].fd = connectedSockfd;        // Sock input
    pfds[1].events = POLLIN;    // Tell me when ready to read
   
    int num_events = poll(pfds, 2, 1); // Wait until an event arrives

    if (pfds[0].revents & POLLIN)
        getline(cin, (*stdin_message));

    if (pfds[1].revents & POLLIN)
    {
        memset(&buffer, 0, sizeof(buffer));
        if (recv(connectedSockfd, &buffer, sizeof(buffer), 0) < 0)
            return -1;
        *socket_message = buffer;
    }

    return 1;
}
