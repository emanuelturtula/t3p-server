#pragma once

#include "types.h"

#define TCP_BUFFER_SIZE 1024

void processClient(int connectedSockfd, int slotNumber);
