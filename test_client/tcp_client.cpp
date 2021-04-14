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


using namespace std;

mutex m;

void readerThread(int connectedSockfd);
void writeOut(const char *message);
void writeError(const char *message);
bool connected = false;


int main(int argc, char *argv[])
{
    int sockfd;
    int bytes;
    string message;
    const char *c_message;
    struct sockaddr_in server = {0};
    const char *ip = argv[1];

    server.sin_family = AF_INET;
    server.sin_port = htons(2000);
    server.sin_addr.s_addr = inet_addr(ip);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) != 0)
        return EXIT_FAILURE;
    
    thread reader(readerThread, sockfd);
    reader.detach();
    int error_code = 0;
    socklen_t error_code_size = sizeof(error_code);
    connected = true;
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    while ((error_code == 0) && connected)
    {
        message = "";
        writeOut("Type a message:");
        getline(cin, message);
        message += " \r\n \r\n";
        c_message = message.c_str();
        bytes = send(sockfd, c_message, strlen(c_message), 0);
        if (bytes < 0)
        {
            cerr << "Error sending message" << endl;
            return EXIT_FAILURE;
        }
        if (message == "LOGOUT \r\n \r\n")
        {
            cout << "LOGOUT" << endl;   
            break;
        }
        message = "Sending: " + message;
        writeOut(message.c_str());
        
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    }
    close(sockfd);
    connected = false;
    return EXIT_SUCCESS;
}

void readerThread(int connectedSockfd)
{
    int bytes;
    int error_code = 0;
    char buffer[1024];
    socklen_t error_code_size = sizeof(error_code);
    getsockopt(connectedSockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    while((error_code == 0) && connected)
    {
        memset(&buffer, 0, sizeof(buffer));
        bytes = recv(connectedSockfd, &buffer, sizeof(buffer), 0);
        if (bytes < 0)
        {
            writeError("Error receiving message");
            break;
        }
        if (bytes > 0)
            writeOut(buffer);
        getsockopt(connectedSockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    }
    connected = false;
}

void writeOut(const char *message)
{
    lock_guard<mutex> lock(m);
    cout << message << endl;
}

void writeError(const char *message)
{
    lock_guard<mutex> lock(m);
    cerr << message << endl;
}