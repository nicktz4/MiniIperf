
#include "libs.h"
#include "utils.h"

static char running = 1;
static void sig_handler(int signal)
{
    if (signal == SIGINT)
    {
        running = 0;
    }
}

int miniIperf_connect(miniIperf_sock_t *socket, const struct sockaddr *address,
                      socklen_t address_len)
{
    if (connect(socket->sd, address, address_len) < 0)
    {
        perror("Error connecting\n");
        return -EXIT_FAILURE;
    }
    printf("Connected\n");

    return 1;
}

miniIperf_sock_t init_miniIperf_TCP_Client(miniIperf_client_options client_options)
{
    miniIperf_sock_t socket;
    miniIperf_TCP_header_t *clientHeader, *serverHeader;
    struct sockaddr_in sin;

    socket = miniIperf_socket(AF_INET, SOCK_STREAM, 0);

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(DEFAULT_TCP_PORT);

    if (client_options.serverip)
    {
        sin.sin_addr.s_addr = inet_addr(client_options.serverip);
    }
    else
    {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    miniIperf_connect(&socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));

    clientHeader = malloc(sizeof(miniIperf_TCP_header_t));
    serverHeader = malloc(sizeof(miniIperf_TCP_header_t));

    setMiniIperfTCPHeaderOptions(clientHeader, rand(), 0, 0, 0, SYN);
    socket.seq_number = ntohl(clientHeader->seq_number);

    printf("AFTO STEELNW:\n");
    printMiniIperfTCPHeader(*clientHeader);
    printf("Number of Bytes: %ld\n", sizeof(*clientHeader));

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");
    send(socket.sd, clientHeader, sizeof(*clientHeader), 0);

    recv(socket.sd, serverHeader, sizeof(*serverHeader), 0);

    if ((ntohs(serverHeader->message_type) == SYN_ACK) && (ntohl(serverHeader->ack_number) == (socket.seq_number) + 1))
    {
        printf("\nClient received SYN ACK signal from server\n");
        printMiniIperfTCPHeader(*serverHeader);
    }
    else
    {

        perror("Error: Did not receive SYN_ACK header\n");
    }
    memset(clientHeader, 0, sizeof(miniIperf_TCP_header_t));
    setMiniIperfTCPHeaderOptions(clientHeader, 0x0000, ntohl(serverHeader->seq_number) + 1, client_options.packetSize, client_options.measureOneWayDelay, ACK);
    socket.seq_number = 0x0000;
    socket.ack_number = ntohs(clientHeader->ack_number);

    printMiniIperfTCPHeader(*clientHeader);

    send(socket.sd, clientHeader, sizeof(*clientHeader), 0);

    printf("Client sent ACK signal to server\n");

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");

    return socket;
}

/* packetSize in Bytes
 * bandwidth in bits/s
 */
int client_miniIperf(miniIperf_client_options client_options)
{
    miniIperf_sock_t sock, TCP_socket;
    struct sockaddr_in sin;
    miniIperf_UDP_header_t *clientHeader;
    miniIperf_TCP_header_t *TCP_clientHeader;

    FILE *fp;
    char *sendbuf;

    ssize_t data_sent;
    size_t read_items = 0;

    time_t start_time;
    struct miniIperf_time current_time;

    uint32_t seq = 1;

    double sendRate = (double)(client_options.bandwidth / 8) / (double)client_options.packetSize;

    printf("Bandwidth: %ld MBit/s\n", client_options.bandwidth / (USEC_PER_SEC));
    printf("Packet Size in Bytes: %d\n", client_options.packetSize);
    printf("sendRate: %f\n", sendRate);

    TCP_socket = init_miniIperf_TCP_Client(client_options);

    /* AF_INET stands for ipv4*/
    sock = miniIperf_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    /* Always reset the struct sockaddr_in before use*/
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    /*Addresses and ports must be assigned in Network Byte Order*/
    sin.sin_port = htons(client_options.server_port);
    sin.sin_addr.s_addr = inet_addr(client_options.serverip);

    if (client_options.file)
    {
        fp = fopen(client_options.file, "r");
        if (!fp)
        {
            perror("Open file for reading");
            return -EXIT_FAILURE;
        }
    }

    clientHeader = malloc(sizeof(miniIperf_UDP_header_t));
    TCP_clientHeader = malloc(sizeof(miniIperf_TCP_header_t));

    printf("@@@@@@@@@@@@@@MiniIperf Client: Sending data@@@@@@@@@@@@\n");
    sendbuf = (char *)malloc(client_options.packetSize + sizeof(miniIperf_UDP_header_t));
    if (!sendbuf)
    {
        perror("Error: Allocate application send buffer\n");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, sig_handler);
    /*Wait time before starting transmission*/
    sleep(client_options.waitingTimeBeforeStartingExperiment);

    memset(sendbuf, 'a', client_options.packetSize + sizeof(*clientHeader));

    start_time = time(NULL);
    sendRate = ((double)client_options.packetSize * 8) / (double)client_options.bandwidth;
    while (running && (difftime(time(NULL), start_time) < client_options.experimentDuration))
    {
        setMiniIperfUDPHeaderOptions(clientHeader, seq++, client_options.packetSize);
        current_time = miniIperfTimeNow();
        clientHeader->timestamp.sec = htonl(current_time.sec);
        clientHeader->timestamp.usec = htonl(current_time.usec);
        memcpy(sendbuf, clientHeader, sizeof(*clientHeader));

        printf("STELNW:\n");
        printMiniIperfUDPHeader(*clientHeader);
        data_sent = sendto(sock.sd, sendbuf, client_options.packetSize + sizeof(*clientHeader), 0, (struct sockaddr *)&sin, sizeof(sin));

        if (data_sent != client_options.packetSize + sizeof(*clientHeader))
        {
            perror("Client failed s to send data\n");
            break;
        }

        if (client_options.bandwidth == 0)
            sleep(0.02);
        else
            usleep(USEC_PER_SEC * sendRate);
    }

    setMiniIperfTCPHeaderOptions(TCP_clientHeader, seq, 0, 0, 0, FIN);
    printMiniIperfTCPHeader(*TCP_clientHeader);
    send(TCP_socket.sd, TCP_clientHeader, sizeof(*TCP_clientHeader), 0);
    printf("@@@@@@@@@@@@@@MiniIperf Client: Terminating Connection@@@@@@@@@@@@@\n");
    free(sendbuf);
    fclose(fp);

    return 1;
}