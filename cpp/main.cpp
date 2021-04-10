#include "../headers/main.h"
#include "../headers/t3p_server.h"
#include "../headers/types.h"
#include "../headers/tests.h"
#include <regex>
#include <iostream>

using namespace std;

int main() 
{
    status_t status;
    Logger logger;
    #if defined(DEBUG_TEST)
        // Just for testing we are hardcoding the IP, but it should be 
        // read from command line as an argument or from config file.
        if((status = t3p_server("127.0.0.1")) != STATUS_OK)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    #else
        #ifndef SERVER_IP
            cerr << "Missing IP in config file.";
            return EXIT_FAILURE;
        #endif
        regex ip_checker("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        const char *ip = SERVER_IP;
        if (!regex_match(ip, ip_checker))
        {
            cerr << "IP in config file does not have a correct format.";
            return EXIT_FAILURE;
        }
        if((status = t3p_server(ip)) != STATUS_OK)
        {
            logger.printMessage("T3P server return with:");
            logger.errorHandler.printErrorCode(status);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    #endif


    return EXIT_SUCCESS;
}

