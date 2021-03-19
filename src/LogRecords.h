#ifndef LOGRECORDS_H
#define LOGRECORDS_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>

#include "ErrorHandler.h"

using namespace std;



class LogRecords {
    public:
        LogRecords();
        void LogMessage(status_t);
        void LogMessage(string);
};

#endif