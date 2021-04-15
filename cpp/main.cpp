#include "../headers/main.h"
#include "../headers/t3p_server.h"
#include "../headers/types.h"
#include "../headers/tests.h"
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

using namespace std;


string serverIp = "";
map<string, int> config = {
    {"TCP_PORT", 0},
    {"UDP_PORT", 0},
    {"TCP_MAX_PENDING_CONNECTIONS", 0},
    {"MAX_PLAYERS_ONLINE", 0},
    {"HEARTBEAT_TIMEOUT", 0},
    {"MARKSLOT_TIMEOUT", 0},
    {"INVITATION_TIMEOUT", 0}
};

bool loadConfig();

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
        if (!loadConfig())
            return EXIT_FAILURE;
        regex ip_checker("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        const char *ip = serverIp.c_str();
        if (!regex_match(ip, ip_checker))
        {
            cerr << "IP in config file does not have a correct format.";
            return EXIT_FAILURE;
        }

        cout << endl;
        cout << "********************************************" << endl;
        cout << BOLDGREEN << "Loaded configuration file correctly" << RESET << endl;
        cout << "Server will start with the following configuration" << endl;
        for (auto const& pair : config)
        {
            cout << pair.first << ": " << pair.second << endl; 
        }
        cout << "********************************************" << endl;
        cout << endl;

        if((status = t3p_server(ip)) != STATUS_OK)
        {
            logger.printMessage("T3P server returned with:");
            logger.errorHandler.printErrorCode(status);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    #endif


    return EXIT_SUCCESS;
}

bool loadConfig()
{
    ifstream fin("config.txt");
    string line;
    while (getline(fin, line)) {
        istringstream sin(line.substr(line.find("=") + 1));
        if (line.find("SERVER_IP") != -1)
        {
            if (serverIp != "")
            {
                cerr << "Error loading configuration file. Key is repeated" << endl;
                return false;
            }
            else
                sin >> serverIp;
        }
        else
        {
            for (auto const& pair : config)
            {
                if (line.find(pair.first) != -1) 
                {
                    if (pair.second == 0)
                        sin >> config[pair.first];
                    else
                    {
                        cerr << "Error loading configuration file. Key is repeated" << endl;
                        return false;
                    }   
                }
            }
        }
    }

    for (auto const& pair : config)
    {
        if (pair.second == 0)
        {
            cerr << "Error loading configuration file. Missing value: " << pair.first << endl;
            return false;
        }
    }
    
    if (serverIp == "")
    {
        cerr << "Error loading configuration file. Missing value: " << "SERVER_IP" << endl;
        return false;
    }

    return true;
}