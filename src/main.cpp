#include "main.h"


#define DEBUG_TEST

int main(void){

    status_t status;

    #if defined(DEBUG_TEST)

        if((status = t3p_server()) != STATUS_OK)
            return 1;
        return 0;

    #else
        if((status = t3p_server()) != STATUS_OK)
            return 1;
        return 0;
    #endif


    return 0;
}