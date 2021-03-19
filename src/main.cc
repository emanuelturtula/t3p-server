#include "main.h"
#include "ErrorHandler.h"
#include "SocketsSetter.h"






int main(void){

    status_t status;
    int sockfd;


    if( (status= settingUDPSocketListener(&sockfd)) != STATUS_OK) {

    }


    return 0;
}