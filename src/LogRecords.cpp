#include "LogRecords.h"


// Should include writing all in a log file in the future. In the meantime, it writes status of program
// in the terminal

LogRecords :: LogRecords(){

}


void LogRecords :: LogMessage(status_t status){
    cout << "Got status ID =" << status << "\n";
}

void LogRecords :: LogMessage(string message){
    cout << message << "\n";
}

void LogRecords :: LogMessage(string message, status_t status){
    this->LogMessage(status);
    this->LogMessage(message);
}