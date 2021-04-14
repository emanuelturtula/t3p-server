#pragma once

#include "types.h"

#define UDP_BUFFER_SIZE 1024

void processUDPMesages(int udpSocket, const char *ip);