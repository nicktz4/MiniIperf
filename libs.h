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
#include <poll.h>
#include <arpa/inet.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#include "miniIperf_time.h"

#define MESSAGE_TYPE 33
#define CHUNK_SIZE 63400
#define DEFAULT_PORT 8080
#define DEFAULT_TCP_PORT 55600
#define USEC_PER_SEC 1000000

typedef enum
{
    UNKNOWN,
    INVALID,
    ESTABLISHED,
} miniIperf_state_t;

typedef enum
{
    APPLICATION_DATA = 33,
    SYN = 34,
    SYN_ACK = 35,
    ACK = 36,
    FIN = 37,
    FIN_ACK = 38,
    RST = 39,
} miniIperf_packet_type_t;

typedef struct
{
    uint32_t seq_number;
    uint32_t data_len; /*(Data length in bytes) (Excluding header)*/
    struct miniIperf_time timestamp;

} miniIperf_UDP_header_t;

typedef struct
{
    uint32_t seq_number;
    uint32_t ack_number;
    uint32_t packet_len; /*(Data length in bytes) (Excluding header)*/
    uint8_t measureOneWayDelay;
    miniIperf_packet_type_t message_type;


}miniIperf_TCP_header_t;

typedef struct
{
    int sd;                  /* socket Descriptor*/
    int server_sd;           /* This socket is used  to recv and send data*/
    miniIperf_state_t state; /* State of the socket*/
    uint16_t packet_len;    /* The window size negotiated at the 3-way handshake*/

    size_t seq_number; /** Keep the state of the sequence number */
    size_t ack_number; /**Keep the state of the ack number */

   

} miniIperf_sock_t;

typedef struct
{
    char *ip;
    uint16_t listen_port;
    char *filestr;
    uint16_t informationInterval;

} miniIperf_server_options;

typedef struct 
{
  /* Statistics */
    uint32_t throughput;
    uint32_t goodput;
    uint64_t jitter;
    uint64_t std_dev_jitter;

    double one_way_delay;
    double packet_loss_percentage;

    struct miniIperf_time start;
    struct miniIperf_time end;

}miniIperf_statistics;


typedef struct
{
    const char *serverip;
    uint16_t server_port;
    const char *file;
    uint16_t packetSize;
    uint64_t bandwidth;
    uint16_t parallelStreamsNumber;
    uint16_t experimentDuration;
    uint8_t measureOneWayDelay;
    uint16_t waitingTimeBeforeStartingExperiment;
    uint16_t informationInterval;
} miniIperf_client_options;

void printMiniIperfUDPHeader(miniIperf_UDP_header_t header);
void printMiniIperfTCPHeader(miniIperf_TCP_header_t header);

int miniIperf_accept(miniIperf_sock_t, struct sockaddr *, socklen_t);

int miniIperf_bind(miniIperf_sock_t *, const struct sockaddr *, socklen_t);

int miniIperf_connect(miniIperf_sock_t *, const struct sockaddr *,
                      socklen_t);

#endif