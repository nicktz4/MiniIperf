
#include "libs.h"

int miniIperf_connect(miniIperf_sock_t *socket, const struct sockaddr *address,
                     socklen_t address_len)
{
    if(connect(socket->sd , address,address_len) < 0 )
    {
        perror("Error connecting\n");
        return -EXIT_FAILURE;
    }
    printf("Connected\n");
    


}