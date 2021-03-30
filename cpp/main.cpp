#include "../headers/main.h"
#include "../headers/t3p_server.h"
#include "../headers/types.h"
#include "../headers/tests.h"

using namespace std;

int main(void){
    status_t status;
    #if defined(DEBUG_TEST)
        // Just for testing we are hardcoding the IP, but it should be 
        // read from command line as an argument or from config file.
        if((status = t3p_server("127.0.0.1")) != STATUS_OK)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    #else
        if((status = t3p_server()) != STATUS_OK)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    #endif


    return EXIT_SUCCESS;
}