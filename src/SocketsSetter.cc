#include "SocketsSetter.h"


status_t settingUDPSocketListener(int *sockfd){
  char yes = 1;
  struct addrinfo hints;
  struct addrinfo *serverinfo, *p; // *p: will run over every getaddrinfo() result and find one suitable
  LogRecords logHandler;

  if(sockfd == NULL){
    return ERROR_UDPLISTENER_SOCKET_PARAMETER_NULL;
  }

  memset(&hints, 0, sizeof hints);    // make sure the struct is empty
  hints.ai_family = AF_UNSPEC;        // don't care IPv4 or IPv6
  hints.ai_socktype = SOCK_DGRAM;     // UDP stream sockets
  hints.ai_flags = AI_PASSIVE;        // Using device own IP for server 


  if ((getaddrinfo(NULL, UDP_PORT_NUMBER, &hints, &serverinfo)) != 0) {
      return ERROR_UDPLISTENER_GETADDRINFO;
  }

  // Loop through all the results and bind to the first we can:
  for(p = serverinfo; p != NULL; p = p->ai_next) {
    if ((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
      logHandler.LogMessage(ERROR_UDPLISTENER_SOCKET_CREATION); 
      continue;
    }

    // lose the pesky "Address already in use" error message
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) { 
      return ERROR_UDPLISTENER_SOCKET_SETSOCKOPT;
    }

    if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(*sockfd);
      logHandler.LogMessage(ERROR_UDPLISTENER_SOCKET_BINDING); 
      continue;
    }
    break; // It didn't  fall in a continue
  }

  freeaddrinfo(serverinfo);

  if (p == NULL) {
    return ERROR_UDPLISTENER_SOCKET_BINDING_FAILURE;
  }

  // As the funciton set up a listener, we put the socket to listening state:
  if (listen(*sockfd, BACKLOG) == -1) {
    return ERROR_UDPLISTENER_SOCKET_LISTEN_FAILURE;
  }

  return STATUS_OK;
}