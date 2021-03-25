#include <iostream>
#include <mutex>
#include "../headers/types.h"
#include "../headers/config.h"


using namespace std;

mutex m_stdout, m_stderr;

/**
 * Methods for Logger
 * */
// Should include writing all in a log file in the future. In the meantime, it writes status of program
// in the terminal
Logger :: Logger()
{

}

void Logger :: printMessage(status_t status)
{
    this->printer.writeStderr("Got status ID = " + to_string(status));
}

void Logger :: printMessage(string message)
{
    this->printer.writeStderr(message);
}

void Logger :: printMessage(string message, status_t status)
{
    this->printMessage(status);
    this->printMessage(message);
}

void Logger :: debugMessage(string message)
{
    #if defined(DEBUG_MESSAGES)
        this->printMessage(message);
    #endif
}


/**
 * Methods for ErrorHandler
 * */
ErrorHandler :: ErrorHandler()
{

}

void ErrorHandler :: printErrorCode(status_t status)
{
    Printer printerObj;
    printerObj.writeStderr("Error ID = " + to_string(status));
}

/**
 * Methods for Printer
 * */
Printer :: Printer()
{

}

// Writing in stdout or stderr might be used simultaneously by several threads, so we want
// to ensure that the streams are protected against that.
void Printer :: writeStdout(string message)
{
    lock_guard<mutex> lock(m_stdout);
    cout << message << endl;
}

void Printer :: writeStderr(string message)
{
    lock_guard<mutex> lock(m_stderr);
    cout << message << endl;
}