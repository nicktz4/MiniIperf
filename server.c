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


int checkForFIN(int sd)
{
    miniIperf_TCP_header_t clientTCPHeader;
    struct pollfd tcp_poll_fd[1];

    memset(tcp_poll_fd, 0, sizeof(tcp_poll_fd));
    tcp_poll_fd->fd = sd;
    tcp_poll_fd->events = POLL_IN;
    poll(tcp_poll_fd, 1, 0);
    if (!(tcp_poll_fd->revents & POLL_IN))
    {
        return 0;
    }
    printf("Why?\n");
    // Check if we have received FIN packet
    if (recv(sd, &clientTCPHeader, sizeof(clientTCPHeader), 0) > 0)
    {

        if ((ntohs(clientTCPHeader.message_type) == FIN) && (ntohl(clientTCPHeader.ack_number) == 0))
        {
            printf("Client received FIN signal from server\n");
            return 1;
        }
    }
}

int miniIperf_bind(miniIperf_sock_t *socket, const struct sockaddr *address, socklen_t address_len)
{
    if (bind(socket->sd, address, address_len) == -1)
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

int miniIperf_accept(miniIperf_sock_t socket, struct sockaddr *address, socklen_t address_len)
{
    if ((socket.server_sd = accept(socket.sd, address, &address_len)) < 0)
    {
        perror("Error accepting connection\n");
        return -EXIT_FAILURE;
    }

    printf("server got a connection from %s from port %d \n", inet_ntoa(((struct sockaddr_in *)address)->sin_addr), ntohs(((struct sockaddr_in *)address)->sin_port));

    return socket.server_sd;
}

miniIperf_sock_t init_miniIperf_TCP_Server(miniIperf_server_options server_options)
{
    miniIperf_sock_t socket;
    miniIperf_TCP_header_t *clientHeader, *serverHeader;
    struct sockaddr_in sin;

    int recv_length = 0;

    socket = miniIperf_socket(AF_INET, SOCK_STREAM, 0);

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(DEFAULT_TCP_PORT);

    if (server_options.ip)
    {
        sin.sin_addr.s_addr = inet_addr(server_options.ip);
    }
    else
    {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    if (miniIperf_bind(&socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Error bInding socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Listening...\n");
    if (listen(socket.sd, 5) < 0)
    {
        perror("Error listening\n");
        exit(EXIT_FAILURE);
    }

    socket.sd = miniIperf_accept(socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));

    clientHeader = malloc(sizeof(miniIperf_TCP_header_t));
    serverHeader = malloc(sizeof(miniIperf_TCP_header_t));

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");
    recv_length = recv(socket.sd, clientHeader, sizeof(*clientHeader), 0);
    printf("Client Header Received::\n");
    printMiniIperfTCPHeader(*clientHeader);
    if (ntohs(clientHeader->message_type) == SYN)
    {
        printf("Server Received SYN packet from Client\n");
        printMiniIperfTCPHeader(*clientHeader);
    }
    else
    {
        perror("Error: Did not Receive SYN header\n");
    }

    setMiniIperfTCPHeaderOptions(serverHeader, rand(), ntohl(clientHeader->seq_number) + 1, 0, 0, SYN_ACK);

    printf("Server Sending SYN_ACK packet to Client\n");
    printMiniIperfTCPHeader(*serverHeader);
    socket.seq_number = ntohl(serverHeader->seq_number);

    send(socket.sd, serverHeader, sizeof(*serverHeader), 0);

    recv_length = recv(socket.sd, clientHeader, sizeof(*clientHeader), 0);
    printf("RECEIVEEEEEEEEEEEEEEEEEEEEEEEEEE:\n");
    printMiniIperfTCPHeader(*clientHeader);

    if (((ntohs(clientHeader->message_type) == ACK) && (ntohl(clientHeader->ack_number) == (socket.seq_number + 1))))
    {
        socket.ack_number = serverHeader->ack_number + 1;
        socket.seq_number = serverHeader->seq_number + 1;

        printf("\nServer received SYN ACK  signal from client\n");
        printMiniIperfTCPHeader(*clientHeader);
    }
    else
    {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    socket.packet_len = ntohs(clientHeader->packet_len);

    printf("@@@@@@@@@@ END 3 WAY HANDSHAKE @@@@@@@@@@@\n");
    return socket;
}

int server_miniIperf(miniIperf_server_options server_options)
{
    miniIperf_sock_t socket, TCP_socket;
    miniIperf_UDP_header_t *clientHeader;
    miniIperf_TCP_header_t *clientTCPHeader;
    socklen_t client_addr_len;
    struct sockaddr_in client_addr;
    int recv_length = 1;
    void *buff;

    struct sockaddr_in sin;

    FILE *fp;

    struct miniIperf_time start_time, end_time, last_time, current_time;
    uint64_t total_bytes_data = 0;
    int received = 0;

    struct miniIperf_time nowTimestamp;
    struct miniIperf_time owdTime;

    uint32_t packet_cnt = 0, packet_cnt_until_print = 0;
    /*The number of all packets the client sent*/
    uint32_t packets_sent;

    /*Variable for 1st packet arrived*/
    int cnt = 0;

    struct miniIperf_time jitter, jitterstart, tmpJitter;
    double averageJitter = 0, varianceJitter = 0, mean;

    uint32_t packets_lost = 0;
    uint64_t bytes_received = 0, packets_received = 0;

    uint64_t bytes_received_until_print = 0, packets_lost_until_print = 0;
    miniIperf_statistics stats;
    TCP_socket = init_miniIperf_TCP_Server(server_options);

    socket = miniIperf_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    socket.packet_len = TCP_socket.packet_len;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(server_options.listen_port);

    if (server_options.ip)
    {
        sin.sin_addr.s_addr = inet_addr(server_options.ip);
    }
    else
    {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    if (miniIperf_bind(&socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Error bInding socket\n");
        exit(EXIT_FAILURE);
    }

    if (server_options.filestr)
    {
        fp = fopen(server_options.filestr, "a+");
        if (fp == NULL)
        {
            perror("Error opening file\n");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, sig_handler);
    client_addr_len = sizeof(client_addr);

    printf("@@@@@@@@@@@@MiniIperf Server iss ready to receive data@@@@@@@@@@@@@@@@@\n");

    printf("Waiting for Client to send data...\n");
    printf("Packet Length: %d\n", socket.packet_len);
    buff = malloc(socket.packet_len + sizeof(miniIperf_UDP_header_t));

    while ((received = recvfrom(socket.sd, buff, socket.packet_len + sizeof(miniIperf_UDP_header_t), 0, (struct sockaddr *)&client_addr, &client_addr_len) ) > 0)
    {
        if(!running)
        {
            break;
        }

        jitterstart = miniIperfTimeNow();

        // Get Client Header
        clientHeader = (miniIperf_UDP_header_t *)buff;
        clientTCPHeader = (miniIperf_TCP_header_t *)buff;

        // printMiniIperfUDPHeader(*clientHeader);

        // Check if a packet is lost or out of order
        if (ntohl(clientHeader->seq_number) >= packet_cnt + 1)
        {
            // That means that we have packet loss
            if (ntohl(clientHeader->seq_number) > packet_cnt + 1)
            {
                // May Add many packets that are lost
                packets_lost += (ntohl(clientHeader->seq_number) - 1) - packet_cnt;
                packets_lost_until_print += (ntohl(clientHeader->seq_number) - 1) - packet_cnt;
            }
            packet_cnt = ntohl(clientHeader->seq_number);
            packet_cnt_until_print = ntohl(clientHeader->seq_number);
        }

        // Calculate One Way Delay
        if (packet_cnt - 1 == 0)
        {
            nowTimestamp = miniIperfTimeNow();
            owdTime.sec = nowTimestamp.sec - ntohl(clientHeader->timestamp.sec);
            owdTime.usec = nowTimestamp.usec - ntohl(clientHeader->timestamp.usec);

            printf("One Way Delay: %ld μs\n", miniIperfTimeInUsec(owdTime));
        }

        // From 2nd packet and then calculate jitter

        if (packet_cnt >= 1)
        {
            jitter = miniIperfTimeDiff(tmpJitter, jitterstart);

            tmpJitter = jitterstart;

            // miniIperfTimePrint(jitter);

            if (jitter.sec > 0)
            {
                averageJitter += (jitter.sec) * USEC_PER_SEC + jitter.usec;
            }
            else
            {
                averageJitter += jitter.usec;
            }

            mean = averageJitter / packet_cnt;
            mean = mean / USEC_PER_SEC;

            varianceJitter += pow((jitter.sec * USEC_PER_SEC + jitter.usec) / USEC_PER_SEC - mean, 2);
            //printf("Standard Deviation Jitter: %f\n",sqrt(varianceJitter/packet_cnt));
        }

        // 3ekiname na metrame ton xrono apo to 2o paketo
        if (cnt == 1)
        {
            cnt++;

            start_time = miniIperfTimeNow();
            last_time = miniIperfTimeNow();
        }

        // Remove Header Size
        total_bytes_data += received - sizeof(miniIperf_UDP_header_t);
        bytes_received += received;

        bytes_received_until_print += received - sizeof(miniIperf_UDP_header_t);
        ++packets_received;

        current_time = miniIperfTimeNow();

        // Kathe informationInterval xrono printaroume ta statistika
        if ((current_time.sec - last_time.sec >= server_options.informationInterval) && cnt > 1)
        {
            end_time = miniIperfTimeNow();
            print_statistics(bytes_received_until_print, bytes_received_until_print, last_time, end_time, packets_lost_until_print, packet_cnt_until_print);
            printf("Jitter: %f μs\n", averageJitter / packet_cnt);
            packets_lost_until_print = 0;
            bytes_received_until_print = 0;
            last_time = current_time;
        }

        if (cnt < 2)
        {
            ++cnt;
        }

        if (checkForFIN(TCP_socket.sd))
        {
            end_time = miniIperfTimeNow();
            printf("Server Received FIN packet from Client\n");
            printMiniIperfTCPHeader(*clientTCPHeader);
            packets_sent = packet_cnt;
            printf("Packets Sent: %d", packets_sent);
            printf("Packet Loss: %d\n", packets_sent - packet_cnt);
        }
    }

    printf("error: %s", strerror(errno));
    printf("@@@@@@@@@@@@@@@MiniIperf Data received!@@@@@@@@@@@@@@@@2\n");

    double elapsed = end_time.sec - start_time.sec + (end_time.usec - start_time.usec) * 1e-12;
    double megabytes = bytes_received / (1024.0 * 1024.0);
    stats.goodput = total_bytes_data / (1024.0 * 1024.0);
    stats.throughput = megabytes / elapsed;
    stats.one_way_delay = miniIperfTimeInUsec(owdTime);
    stats.start = start_time;
    stats.end = end_time;
    stats.jitter = averageJitter / packet_cnt;
    stats.std_dev_jitter = sqrt(varianceJitter / packet_cnt);
    stats.packet_loss_percentage = packets_lost / packet_cnt;
    if(server_options.filestr)
    {
        printf("filestr:%s\n", server_options.filestr);
        statistics_to_file(server_options.filestr, stats);
    }else
    {

        print_statistics(bytes_received, total_bytes_data, start_time, end_time, packets_lost, packet_cnt);
        printf("Mean: %fμs\n",mean);
        printf("Standard Deviation Jitter: %fμs\n",sqrt(varianceJitter/packet_cnt));
    }
    return 1;
}
