#pragma once

#include "types.h"

#define TCP_BUFFER_SIZE 1024
#define SOCKET_RECEIVE_TIMEOUT 1

void processClient(int connectedSockfd, int slotNumber);
