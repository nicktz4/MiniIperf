#ifndef UTILS_H
#define UTILS_H
#include "libs.h"

int setMiniIperfTCPHeaderOptions(miniIperf_TCP_header_t *, uint32_t , uint32_t , uint32_t ,uint8_t , miniIperf_packet_type_t);
int setMiniIperfUDPHeaderOptions(miniIperf_UDP_header_t *header, uint32_t seq_number, uint32_t data_len);

void printMiniIperfTCPHeader(miniIperf_TCP_header_t header);
void printMiniIperfUDPHeader(miniIperf_UDP_header_t header);

void print_statistics(ssize_t received,ssize_t received_data, struct miniIperf_time start, struct miniIperf_time end,ssize_t packets_lost,ssize_t packets);

miniIperf_sock_t miniIperf_socket(int domain, int type, int protocol);

#endif