#include "libs.h"



int miniIperf_bind(miniIperf_sock_t *socket,const struct sockaddr *address,socklen_t address_len)
{
    if ( bind(socket->sd, address , address_len)==-1)
    {
        perror("Error binding socket\n");
        return -EXIT_FAILURE;
    }
    else
    {
        printf("Socket binded\n");
    }
    
    return 1;
}

int miniIperf_accept(miniIperf_sock_t socket,struct sockaddr *address,socklen_t address_len)
{
    if((socket.server_sd = accept(socket.sd,address,&address_len))< 0)
    {
        perror("Error accepting connection\n");
        return -EXIT_FAILURE;
    }

    printf("server got a connection from %s from port %d \n", inet_ntoa(((struct sockaddr_in *)address)->sin_addr), ntohs(((struct sockaddr_in *)address)->sin_port));

    return socket.server_sd;
}

