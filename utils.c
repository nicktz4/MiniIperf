#include "utils.h"


void print_statistics(ssize_t received,ssize_t received_data, struct miniIperf_time start, struct miniIperf_time end,ssize_t packets_lost,ssize_t packets)
{
    double elapsed = end.sec - start.sec + (end.usec - start.usec) * 1e-12;
    double megabytes = received / (1024.0 * 1024.0);
    double goodput = received_data / (1024.0 * 1024.0);

    
    printf("********************** Statistics ******************\n");
    printf("Start Time: %d.%06d\n", start.sec, start.usec);
    printf("End Time: %d.%06d\n", end.sec, end.usec);
    printf("Data received: %f MB\n", goodput);
    printf("Overall received: %f MB\n", megabytes);
    printf("Transfer time: %f seconds\n", elapsed);
    printf("Throughput achieved: %f MB/s\n", megabytes / elapsed);
    printf("Throughput Goodput achieved: %f MB/s\n", goodput / elapsed);
    printf("Number of Packets Client Sent: %ld\n",packets);
    printf("Number of Packets Lost: %ld\n",packets_lost);
    printf("Packet Loss Percentage: %f\n",(double)((double) packets_lost/ (double) packets));
}

void printMiniIperfTCPHeader(miniIperf_TCP_header_t header)
{
    printf("SYN: %u ,"
           "ACK: %u , "
           "Data Length (Packet Length): %u ,"
           "Measure One Way Delay: %u ,"
           "Message Type: %u ,\n",
           ntohl(header.seq_number), ntohl(header.ack_number), ntohs(header.packet_len), ntohs(header.measureOneWayDelay),ntohs(header.message_type));
}
void printMiniIperfUDPHeader(miniIperf_UDP_header_t header)
{
    printf("SYN: %u ,"
           "Data Length: %u ,"
           "TimeStamp Sec: %u,"
           "TimeStamp Usec: %u\n",
           ntohl(header.seq_number), ntohl(header.data_len), ntohl(header.timestamp.sec),ntohl(header.timestamp.usec));
}

int setMiniIperfTCPHeaderOptions(miniIperf_TCP_header_t *header, uint32_t seq_number, uint32_t ack_number, uint32_t packet_len,uint8_t measureOneWayDelay, miniIperf_packet_type_t message_type)
{
    header->seq_number = htonl(seq_number);
    header->ack_number = htonl(ack_number);
    header->measureOneWayDelay = measureOneWayDelay;
    header->packet_len = htons(packet_len);
    header->message_type = htons(message_type);
    return 1;
}

int setMiniIperfUDPHeaderOptions(miniIperf_UDP_header_t *header, uint32_t seq_number, uint32_t data_len)
{
    header->seq_number = htonl(seq_number);
    header->data_len = htonl(data_len);
    return 1;
}

miniIperf_sock_t miniIperf_socket(int domain, int type, int protocol)
{
    miniIperf_sock_t sock;
    int yes = 1;

    if ((sock.sd = socket(domain, type, protocol)) == -1)
    {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Socket created\n");

    if (setsockopt(sock.sd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(yes)) == -1)
    {
        perror("Error setting socket options\n");
        exit(EXIT_FAILURE);
    }
    
    return sock;
}

void statistics_to_file(const char *fn,const miniIperf_statistics *s)
{
    FILE *fp;
    fp = fopen(fn,"a");

    

        fprintf(fp,"Start Time: %d.%06d\n", s->start.sec, s->start.usec);
        fprintf(fp,"End Time: %d.%06d\n", s->end.sec, s->end.usec);
        fprintf(fp,"Jitter: %f\n", s->jitter);
        fprintf(fp,"standard Deviation of Jitter: %f\n", s->std_dev_jitter);
        fprintf(fp,"Throughput achieved: %lu MB/s\n",s->throughput);
        fprintf(fp,"Throughput Goodput achieved: %lu MB/s\n",s->goodput);
        fprintf(fp,"Packet Loss Percentage: %f\n",s->packet_loss_percentage);

    fclose(fp);

}