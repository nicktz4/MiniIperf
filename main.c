#include "libs.h"

static char running = 1;
static void sig_handler(int signal)
{
    if(signal == SIGINT)
    {
        running = 0;
    }
}

static inline void
print_statistics(ssize_t received, struct timespec start, struct timespec end)
{
  double elapsed = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) * 1e-9;
  double megabytes = received / (1024.0 * 1024.0);
  printf("Data received: %f MB\n", megabytes);
  printf("Transfer time: %f seconds\n", elapsed);
  printf("Throughput achieved: %f MB/s\n", megabytes / elapsed);
}

void printMiniIperfHeader(miniIperf_header_t header)
{
    printf("SYN: %d ,"
           "ACK: %d , "
           "Data Length: %d ,"
           "Checksum: %d ,"
           "Window Size: %d ,"
           "Message Type: %d \n",
           ntohl(header.seq_number), ntohl(header.ack_number), ntohl(header.data_len), ntohl(header.checksum), ntohs(header.windowSize), ntohs(header.message_type));
}

int setMiniIperfHeaderOptions(miniIperf_header_t *header, uint32_t seq_number, uint32_t ack_number, uint32_t data_len, uint32_t checksum, uint16_t windowSize, miniIperf_packet_type_t message_type)
{
    header->seq_number = htonl(seq_number);
    header->ack_number = htonl(ack_number);
    header->data_len = htonl(data_len);
    header->checksum = htonl(checksum);
    header->windowSize = htons(windowSize);
    header->message_type = htons(message_type);
    return 1;
}

miniIperf_sock_t miniIpert_socket(int domain, int type, int protocol)
{
    miniIperf_sock_t sock;
    uint8_t yes = 1;

    if ((sock.sd = socket(domain, type, protocol)) == -1)
    {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Socket created\n");

    if (setsockopt(sock.sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Error setting socket options\n");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int server_miniIperf(char *ip, uint16_t listen_port, char *filestr, uint16_t informationInterval)
{
    char buffer[1024];
    miniIperf_sock_t socket;
    miniIperf_header_t *clientHeader;
    miniIperf_header_t header_host;
    miniIperf_header_t *serverHeader;
    socklen_t client_addr_len;
    struct sockaddr_in client_addr;
    int recv_length = 1;
    uint8_t *data;

    struct sockaddr_in sin;

    FILE *fp;

    struct timespec start_time,end_time;
    ssize_t written,total_bytes=0;
    int received;

    time_t last_time;
    time_t current_time;

    int cnt=0;

    socket = miniIpert_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listen_port);

    if (ip)
    {
        sin.sin_addr.s_addr = inet_addr(ip);
    }
    else
    {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    if (miniIperf_bind(&socket, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Error binding socket\n");
        exit(EXIT_FAILURE);
    }

    if (filestr)
        fp = fopen(filestr, "wb");
    if (fp == NULL)
    {
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
    }

    memset(&buffer, 0, sizeof(buffer));

    if (!buffer)
    {
        fclose(fp);
        perror("Error allocating memory\n");
        exit(EXIT_FAILURE);
    }

    /*Blocking Call.If a connection arrives , then
    a socket descriptor is returned (socket->server_sd)
    to receive and send data*/

    /*socket.server_sd = miniIperf_accept(socket, &client_addr, client_addr_len);*/

    clientHeader = malloc(sizeof(miniIperf_header_t));
    serverHeader = malloc(sizeof(miniIperf_header_t));

    client_addr_len = sizeof(client_addr);

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");
    recv_length = recvfrom(socket.sd, buffer, sizeof(buffer), 0, &client_addr, &client_addr_len);

    clientHeader = (miniIperf_header_t *)buffer;

    if (ntohs(clientHeader->message_type) == SYN)
    {
        printf("Server Received SYN packet from Client\n");
        printMiniIperfHeader(*clientHeader);
    }
    else
    {
        perror("Error: Did not Receive SYN header\n");
    }

    setMiniIperfHeaderOptions(serverHeader, rand(), ntohl(clientHeader->seq_number) + 1, 0, 0, 0, SYN_ACK);

    printf("Server Sending SYN_ACK packet to Client\n");
    printMiniIperfHeader(*serverHeader);
    socket.seq_number = ntohl(serverHeader->seq_number);

    memset(&buffer, 0, sizeof(buffer));

    memcpy(buffer, serverHeader, sizeof(*serverHeader));

    sendto(socket.sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

    recv_length = recvfrom(socket.sd, buffer, sizeof(buffer), 0, &client_addr, &client_addr_len);
    clientHeader = (miniIperf_header_t *)buffer;
    if (((ntohs(clientHeader->message_type) == ACK) && (ntohl(clientHeader->ack_number) == (socket.seq_number + 1))))
    {
        socket.ack_number = serverHeader->ack_number + 1;
        socket.seq_number = serverHeader->seq_number + 1;

        printf("\nServer received SYN ACK  signal from client\n");
        printMiniIperfHeader(*clientHeader);
    }
    else
    {
        perror("ERROR");
    }

    socket.recvbuf = malloc(CHUNK_SIZE * sizeof(char));
    socket.init_win_size = clientHeader->windowSize;
    socket.curr_win_size = clientHeader->windowSize;

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");


    printf("@@@@@@@@@@@@MiniIperf Server is ready to receive data@@@@@@@@@@@@@@@@@\n");

    socket.recvbuf = (uint8_t*)malloc(CHUNK_SIZE);


    if (filestr)
    {

    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    while( (received = recvfrom(socket.sd, socket.recvbuf, CHUNK_SIZE, 0, &client_addr, &client_addr_len)) > 0)
    {
        written = fwrite(socket.recvbuf,sizeof(uint8_t),received,fp);
        total_bytes +=received;
        if(written * sizeof(uint8_t) !=received)
        {
            printf("Failed to write to the file the"
			       " amount of data received from the network.OR FINACK\n");
			break;
        }
        
        clientHeader = (miniIperf_header_t *)socket.recvbuf;
        if ((ntohs(clientHeader->message_type) == FIN) && (ntohl(clientHeader->ack_number) ==0 ))
        {
            printf("Server Received FIN packet from Client\n");
            printMiniIperfHeader(*clientHeader);
            break;
        }

        
    }
    }
    else
    {



        
        while( (received = recvfrom(socket.sd, socket.recvbuf, CHUNK_SIZE, 0, &client_addr, &client_addr_len)) > 0)
        {
            /*3ekiname na metrame ton xrono apo to 2o paketo*/
            if(cnt==1)
            {
                cnt++;
                clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);

                clock_gettime(CLOCK_MONOTONIC_RAW,&last_time);
            }
            clock_gettime(CLOCK_MONOTONIC_RAW,&current_time);
            total_bytes +=received;
            
            /*Kathe informationInterval xrono printaroume ta statistika*/
            if(current_time - last_time >= informationInterval && cnt>1)
            {
                clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
                print_statistics(total_bytes,start_time,end_time);
                last_time = current_time;
            }

            clientHeader = (miniIperf_header_t *)socket.recvbuf;
            if ((ntohs(clientHeader->message_type) == FIN) && (ntohl(clientHeader->ack_number) ==0 ))
            {
                printf("Server Received FIN packet from Client\n");
                printMiniIperfHeader(*clientHeader);
                break;
            }
            if(cnt<2)
            {
                ++cnt;
            }
        }
    }
    printf("error: %s",strerror(errno));
    clock_gettime(CLOCK_MONOTONIC_RAW,&end_time);
    printf("@@@@@@@@@@@@@@@MiniIperf Data received!@@@@@@@@@@@@@@@@2\n");
    print_statistics(total_bytes,start_time,end_time);
    fclose(fp);
    free(socket.recvbuf);
        return 1;
}

int client_miniIperf(const char *serverip, uint16_t server_port, const char *file,
                     size_t packetSize, uint64_t bandwidth, uint16_t parallelStreamsNumber, uint16_t experimentDuration, uint8_t measureOneWayDelay, uint16_t waitingTimeBeforeStartingExperiment, uint16_t informationInterval)
{
    miniIperf_sock_t sock;
    struct sockaddr_in sin;
    miniIperf_header_t *clientHeader;
    miniIperf_header_t *serverHeader;

    socklen_t server_addr_len;
    struct sockaddr_in server_addr;
    FILE *fp;
    char buffer[1024];
    uint8_t *sendbuf;

    ssize_t data_sent;
    size_t read_items = 0;

    time_t start_time;

    /* AF_INET stands for ipv4*/
    /* SOCK_STREAM (TCP)*/
    sock = miniIpert_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    /* Always reset the struct sockaddr_in before use*/
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    /*Addresses and ports must be assigned in Network Byte Order*/
    sin.sin_port = htons(server_port);
    sin.sin_addr.s_addr = inet_addr(serverip);

    /*
        if (miniIperf_connect(&sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1)
        {
            perror("TCP Connection Failed");
            exit(EXIT_FAILURE);
        }
        */
    printf("TCP Connection Established\n");

    /* Allocate memory for the application receive buffer */
    memset(buffer, 0, sizeof(buffer));
    /*memcpy(buffer+sizeof(clientHeader),data,data_len);*/

    if (!buffer)
    {
        perror("Allocate application receive buffer");
        return -EXIT_FAILURE;
    }

    if (file)
        fp = fopen(file, "r");
    if (!fp)
    {
        perror("Open file for reading");
        return -EXIT_FAILURE;
    }

    clientHeader = malloc(sizeof(miniIperf_header_t));
    serverHeader = malloc(sizeof(miniIperf_header_t));

    setMiniIperfHeaderOptions(clientHeader, rand(), 0, 0, 0, 0, SYN);
    sock.seq_number = ntohl(clientHeader->seq_number);

    printf("AFTO STEELNW:\n");
    printMiniIperfHeader(*clientHeader);
    printf("Number of Bytes: %d\n", sizeof(*clientHeader));

    memcpy(buffer, clientHeader, sizeof(*clientHeader));

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");
    sendto(sock.sd, buffer, sizeof(*clientHeader), 0, (struct sockaddr *)&sin, sizeof(sin));

    server_addr_len = sizeof(server_addr);

    sock.buf_fill_level = recvfrom(sock.sd, buffer, sizeof(miniIperf_header_t), 0, &server_addr, &server_addr_len);
    serverHeader = (miniIperf_header_t *)buffer;

    if ((ntohs(serverHeader->message_type) == SYN_ACK) && (ntohl(serverHeader->ack_number) == (sock.seq_number) + 1))
    {
        printf("\nClient received SYN ACK signal from server\n");
        printMiniIperfHeader(*serverHeader);
    }
    else
    {

        perror("Error: Did not receive SYN_ACK header\n");
    }
    memset(clientHeader, 0, sizeof(miniIperf_header_t));
    setMiniIperfHeaderOptions(clientHeader, 0x0000, ntohl(serverHeader->seq_number) + 1, 0xff00, 0, packetSize, ACK);
    sock.seq_number = 0x0000;
    sock.ack_number = ntohs(clientHeader->ack_number);

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, clientHeader, sizeof(*clientHeader));
    sendto(sock.sd, buffer, sizeof(miniIperf_header_t), 0, (struct sockaddr *)&sin, sizeof(sin));

    printf("Client sent ACK signal to server\n");
    printMiniIperfHeader(*clientHeader);

    sock.recvbuf = malloc(packetSize * sizeof(char));
    sock.init_win_size = packetSize;
    sock.curr_win_size = packetSize;

    printf("@@@@@@@@@@ 3 WAY HANDSHAKE @@@@@@@@@@@\n");


    
    printf("@@@@@@@@@@@@@@MiniIperf Client: Sending data@@@@@@@@@@@@\n");
    sendbuf = (uint8_t *)malloc(packetSize);
    if(!sendbuf)
    {
        perror("Error: Allocate application send buffer\n");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT,sig_handler);
    /*Wait time before starting transmission*/
    sleep(waitingTimeBeforeStartingExperiment);
    if(file) {
    start_time = time(NULL);
    
    while(!feof(fp) && running && (difftime(time(NULL),start_time) < experimentDuration))
    {
        read_items = fread(sendbuf,sizeof(uint8_t),packetSize,fp);
        printf("Read Items: %d\n",read_items);
        if(read_items < 1 && errno!=EAGAIN)
        {
            perror("Client failed to read from file\n");
            break;
        }
        data_sent = sendto(sock.sd,sendbuf,read_items*sizeof(uint8_t),0,(struct sockaddr *)&sin,sizeof(sin));
        printf("Data Sent: %d\n",data_sent);
        if(data_sent !=read_items*sizeof(uint8_t))
        {
            perror("Client failed s to send data\n");
            break;
        }
        sleep(0.02);
    }
    }
    else
    {
        memset(sendbuf,'a',packetSize);

    start_time = time(NULL);
    while(running && (difftime(time(NULL),start_time) < experimentDuration))
    {
        data_sent = sendto(sock.sd,sendbuf,packetSize,0,(struct sockaddr *)&sin,sizeof(sin));
        printf("Data Sent: %d\n",data_sent);
        if(data_sent !=packetSize)
        {
            perror("Client failed s to send data\n");
            break;
        }
        sleep(0.02);
    }

    }

    setMiniIperfHeaderOptions(clientHeader, 0x0000, 0x0000, 0x0000, 0,0x0000, FIN);
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, clientHeader, sizeof(*clientHeader));
    sendto(sock.sd, buffer, sizeof(miniIperf_header_t), 0, (struct sockaddr *)&sin, sizeof(sin));
    printf("@@@@@@@@@@@@@@MiniIperf Client: Terminating Connection@@@@@@@@@@@@@\n");
    free(buffer);
    free(sendbuf);
    fclose(fp);

    return 1;
}

int main(int argc, char *argv[])
{
    int opt;
    uint16_t port;
    char *filestr = NULL;
    char *ipstr = NULL;
    uint8_t is_server = 0;
    uint8_t is_client = 0;
    uint8_t measureOneWayDelay = 0;
    uint64_t bandwidth;
    uint16_t parallelStreamsNumber;
    size_t packetSize=-1;
    uint16_t experimentDuration;
    int exit_code = 0;
    uint16_t informationInterval;
    uint16_t waitingTimeBeforeStartingExperiment;
    while ((opt = getopt(argc, argv, "hscdl:b:n:t:w:f:p:a:i:")) != -1)
    {
        switch (opt)
        {
        case 's':
            printf("server mode\n");
            if (is_client == 1)
            {
                printf("Cannot be both server and client\n");
                printf("EXITING..\n");
                exit(1);
            }
            is_server = 1;
            break;
        case 'c':
            printf("client mode\n");
            if (is_server == 1)
            {
                printf("Cannot be both server and client\n");
                exit(1);
            }
            is_client = 1;
            break;
        case 'f':
            printf("File name: %s\n", strdup(optarg));
            filestr = strdup(optarg);
            break;
        case 'p':
            printf("Port: %d\n", atoi(optarg));
            port = atoi(optarg);
            break;
        case 'a':
            printf("IP: %s\n", strdup(optarg));
            ipstr = strdup(optarg);
            break;
        case 'l':
            printf("Packet size: %d\n", atoi(optarg));
            
            packetSize = atoi(optarg);
            
            break;
        case 'b':
            printf("Bandwidth in bits/s: %d\n", atoi(optarg));
            bandwidth = atoi(optarg);
            break;
        case 'n':
            printf("Number of parallel streams: %d\n", atoi(optarg));
            parallelStreamsNumber = atoi(optarg);
            break;
        case 't':
            printf("Experiment duration in seconds:%d\n", atoi(optarg));
            experimentDuration = atoi(optarg);
            break;
        case 'd':
            printf("Measure one way delay\n");
            measureOneWayDelay = 1;
            break;
        case 'w':
            printf("Waiting time before starting experiment in seconds: %d\n", atoi(optarg));
            waitingTimeBeforeStartingExperiment = atoi(optarg);
            break;
        case 'i':
            printf("Information interval in seconds: %d\n", atoi(optarg));
            informationInterval = atoi(optarg);
            break;
        }
    }
    if (is_client == 0 && is_server == 0)
    {
        printf("Please specify if you want to be a client or a server\n");
        printf("EXITING...\n");
        exit(1);
    }
    if (!port)
    {
        port = DEFAULT_PORT;
    }

    printf("Port: %d\n", port);

    srand(time(0));
    if (is_server)
    {

        exit_code = server_miniIperf(ipstr, port, filestr, informationInterval);
    }
    else
    {
        if(!ipstr)
        {
            printf("Please specify the IP address of the server\n");
            printf("EXITING...\n");
            exit(1);
        }
        if(packetSize == -1)
            packetSize = CHUNK_SIZE;
        exit_code = client_miniIperf(ipstr, port, filestr, packetSize, bandwidth, parallelStreamsNumber, experimentDuration, measureOneWayDelay, waitingTimeBeforeStartingExperiment, informationInterval);
    }
}