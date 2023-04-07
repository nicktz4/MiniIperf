#ifndef LIBS_H
#define LIBS_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <assert.h>
#include <signal.h>
#include "utils.h"

#define MESSAGE_TYPE 33
#define CHUNK_SIZE 16384
#define DEFAULT_PORT 8080

typedef enum
{
    UNKNOWN,
    INVALID,
    ESTABLISHED,
}miniIperf_state_t;

typedef enum
{
    APPLICATION_DATA = 33,
    SYN = 34,
    SYN_ACK = 35,
    ACK = 36,
    FIN = 37,
    FIN_ACK = 38,
    RST = 39,
}miniIperf_packet_type_t;


typedef struct
{
    uint32_t seq_number;
    uint32_t ack_number;
    uint32_t data_len; /*(Data length in bytes) (Excluding header)*/
    uint32_t checksum;
    uint16_t windowSize;
    miniIperf_packet_type_t message_type;

} miniIperf_header_t;

typedef struct 
{
    int sd; /* UDP socket Descriptor*/
    int server_sd; /* This socket is used from server to recv and send data*/
    miniIperf_state_t state; /* State of the socket*/
    size_t init_win_size; /* The window size negotiated at the 3-way handshake*/
    size_t curr_win_size; /* Current Window Size*/
    uint8_t *recvbuf; /* Receive Buffer*/
    size_t buf_fill_level; /* Amount of data in the buffer*/

    size_t seq_number; /** Keep the state of the sequence number */
    size_t ack_number; /**Keep the state of the ack number */


    /* Statistics */
    uint64_t packets_send;
    uint64_t packets_received;
    uint64_t packets_lost;
    uint64_t bytes_send;
    uint64_t bytes_received;
    uint64_t bytes_lost;

}miniIperf_sock_t;


void printMiniIperfHeader(miniIperf_header_t header);


int miniIperf_accept(miniIperf_sock_t,struct sockaddr *,socklen_t );

int miniIperf_bind(miniIperf_sock_t *,const struct sockaddr *,socklen_t );

int miniIperf_connect(miniIperf_sock_t *, const struct sockaddr *,
                     socklen_t);

#endif