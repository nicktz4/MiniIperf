#include "libs.h"

int client_miniIperf(miniIperf_client_options);

int server_miniIperf(miniIperf_server_options);

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
    static char setBandwidth = 0;
    uint16_t parallelStreamsNumber;
    size_t packetSize = -1;
    uint16_t experimentDuration=10;

    uint16_t informationInterval;
    uint16_t waitingTimeBeforeStartingExperiment;

    miniIperf_server_options server_options;
    miniIperf_client_options client_options;

    while ((opt = getopt(argc, argv, "scdl:b:n:t:w:f:p:a:i:")) != -1)
    {
        switch (opt)
        {
        case 's':
            printf("server mode\n");
            if (is_client == 1)
            {
                printf("Cannot be both server and client\n");
                printf("EXITING..\n");
            }
            is_server = 1;
            break;
        case 'c':
            printf("client mode\n");
            if (is_server == 1)
            {
                printf("Cannot be both server and client\n");
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
            setBandwidth = 1;
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
        default:
            printf("Usage for server mini_iperf -s [act as server] -a [address] -p [port] -i"
                   " [interval] -f [filename]  .\n"
                   "Usage for client mini_iperf -c [act as client] -a [address] -p [port] \n"
                   "-i [interval] -f [filename] \n"
                   "-l [payload in bytes] -b [bandwidth per sec's] \n"
                   "-n [number of parallel streams] [NOT WORKING] \n"
                   "-t [experiment duration in seconds] \n"
                   "-d [measure one way delay] \n"
                   "-w [wait duration in seconds] \n");
            exit(EXIT_FAILURE);
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
        server_options.listen_port = port;
        server_options.filestr = filestr;
        server_options.informationInterval = informationInterval;
        server_options.ip = ipstr;

        server_miniIperf(server_options);
    }
    else
    {
        if (!ipstr)
        {
            printf("Please specify the IP address of the server\n");
            printf("EXITING...\n");
            exit(1);
        }

        if (setBandwidth)
            client_options.bandwidth=bandwidth;
        else
            client_options.bandwidth=0;

        client_options.serverip = ipstr;
        client_options.server_port = port;
        client_options.file = filestr;
        if(packetSize==-1)
            client_options.packetSize = CHUNK_SIZE;
        else
            client_options.packetSize = packetSize;

        client_options.parallelStreamsNumber = parallelStreamsNumber;
        client_options.experimentDuration = experimentDuration;
        client_options.measureOneWayDelay = measureOneWayDelay;
        client_options.waitingTimeBeforeStartingExperiment = waitingTimeBeforeStartingExperiment;
        client_options.informationInterval = informationInterval;

        client_miniIperf(client_options);
    }
}