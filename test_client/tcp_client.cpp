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

using namespace std;

int main()
{
    int sockfd;
    int bytes;
    string message;
    const char *c_message;
    char buffer[1024];
    struct sockaddr_in server = {0};
    const char *ip = "127.0.0.1";

    server.sin_family = AF_INET;
    server.sin_port = htons(2000);
    server.sin_addr.s_addr = inet_addr(ip);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) != 0)
        return EXIT_FAILURE;

    while (1)
    {
        memset(&buffer, 0, sizeof(buffer));
        message = "";
        getline(cin, message);
        message += " \r\n \r\n";
        c_message = message.c_str();
        send(sockfd, c_message, strlen(c_message), 0);
        bytes = recv(sockfd, &buffer, sizeof(buffer), 0);
        if (bytes > 0)
            cout << buffer;
    }


    return EXIT_SUCCESS;
}