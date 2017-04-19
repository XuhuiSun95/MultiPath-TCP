#include "mptcp.h"

// global variables 
FILE *fp;
long lSize;
int n_paths, port;
char *hostname, *filename, *buffer;
char optchar;
int sockfd[16], ports[16], offset, ack, seq;
socklen_t client_len[16];
ssize_t recv_len;
struct hostent *server;
struct sockaddr_in serveraddr[17], clientaddr[17];
struct mptcp_header hdr[17];
struct packet pkt[17];
char buf[17][MSS];
char *token;
pthread_t th[17];
pthread_mutex_t send_mutex;

// command line helper function
void print_help()
{
    fprintf(stderr, "invalid or missing options\nusage: mptcp [ -n num_interfaces ] [ -h hostname ] [ -p port ] [ -f filename ]\n");
    exit(0);
}

// stderr and exit program
void print_error(char *msg) {
    perror(msg);
    exit(1);
}

// sockfd to index translator
int fd_to_index(const int fd[], int size, int value) {
    int index = 0;
    while ( index < size && fd[index] != value ) ++index;
    return ( index == size ? -1 : index );
}

// mptcp send pthread
void *write_thread_mptcp(void *void_ptr) {
    int fd = *((int *)void_ptr);
    int getindex = fd_to_index(sockfd, n_paths, fd);

    while(1){
        pthread_mutex_lock(&send_mutex);
        if(offset - ack >= RWIN) {
            pthread_mutex_unlock(&send_mutex);
            continue;
        } else if(offset >= lSize) {
            pthread_mutex_unlock(&send_mutex);
            sleep(1);
            continue;
        } else {
            // build pkt and send thread
            bzero(buf[getindex], MSS);
            bzero((char *) &clientaddr[getindex], sizeof(struct sockaddr_in));
            bzero((char *) &hdr[getindex], sizeof(struct mptcp_header));
            bzero((char *) &pkt[getindex], sizeof(struct packet));

            if(getsockname(fd, (struct sockaddr *)&clientaddr[getindex], &client_len[getindex]) < 0)
                print_error("ERROR getting subflow address");

            hdr[getindex].dest_addr = serveraddr[getindex];
            hdr[getindex].src_addr = clientaddr[getindex];
            hdr[getindex].seq_num = offset + 1;
            hdr[getindex].ack_num = 0;
            hdr[getindex].total_bytes = lSize;

            strncpy(buf[getindex], buffer + offset, MSS-1);
            buf[getindex][MSS-1] = '\0';

            pkt[getindex].header = &hdr[getindex];
            pkt[getindex].data = buf[getindex];

            offset += MSS - 1;
            
            // ======= print pkt info ==========  (add if necessary)
            // print_pkt(&pkt[getindex]);

            if(mp_send(sockfd[getindex], (struct packet *)&pkt[getindex], strlen(buf[getindex]), 0) != strlen(buf[getindex]))
                print_error("ERROR sending files on subflow\n");

            pthread_mutex_unlock(&send_mutex);
        }
    }
}

// mptcp read pthread
void *read_thread_mptcp(void *void_ptr) {
    int last_recv_ack = 0;
    int i;
    int tx = 1;

    fd_set read_fdset;
    struct timeval tv;
    int recvfd, recv_ack;

    tv.tv_sec = 3;
    tv.tv_usec = 0;

    // read thread
    while(tx) {
        FD_ZERO(&read_fdset);
        for(i = 0; i < n_paths; i++)
            FD_SET(sockfd[i], &read_fdset);
        
        recvfd = select(sockfd[n_paths - 1] + 1, &read_fdset, NULL, NULL, &tv);
        if(recvfd < 0)
            print_error("ERROR selecting socketfd to read by select()");
        if(recvfd == 0) {
            offset = ack;
            continue;
        }

        // update seq and ack recved
        for(i = 0; i < n_paths; i++) {
            bzero(buf[16], MSS);
            bzero((char *) &hdr[16], sizeof(struct mptcp_header));
            bzero((char *) &pkt[16], sizeof(struct packet));

            pkt[16].header = &hdr[16];
            pkt[16].data = buf[16];

            if(!FD_ISSET(sockfd[i], &read_fdset))
                continue;
            recv_len = mp_recv(sockfd[i], (struct packet *)&pkt[16], MSS, 0);
            if(recv_len < 0)
                print_error("ERROR receiving acks");
            
            recv_ack = pkt[16].header->ack_num;
            if(recv_ack == -1){
                tx = 0;
            } else if(recv_ack == last_recv_ack) {
                ack = recv_ack - 1;
                offset = recv_ack - 1;
            } else if(recv_ack > ack) {
                ack = recv_ack - 1;
            }

            if(ack > offset)
                offset = ack;
            last_recv_ack = pkt[16].header->ack_num;
        }
    }
    return 0;
}

int main(int argc, char ** argv)
{
    // set timer
    clock_t begin = clock();

    // option switch
    opterr = 0;
    while((optchar = getopt(argc, argv, "n:h:p:f:")) != -1)
        switch (optchar) {
            case 'n':
                n_paths = atoi(optarg);
                // printf("number of interfaces: %d\n", n_paths);
                break;
            case 'h':
                hostname = optarg;
                // printf("hostname: %s\n", hostname);
                break;
            case 'p':
                port = atoi(optarg);
                // printf("port number: %d\n", port);
                break;
            case 'f':
                filename = optarg;
                // printf("filename: %s\n", filename);
                break;
            case '?':
                if(strchr("nhpf", optopt) != NULL)
                    print_help();
                else if(isprint(optopt))
                    print_help();
                else 
                    print_help();
            default:
                abort();
        }
    
    // file reader
    fp = fopen(filename, "rb");
    if(fp == NULL)
        print_error("ERROE opening input file\n");
    
    fseek(fp, 0, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize + 1);
    if(!buffer) {
        fclose(fp);
        print_error("ERROR file buffer allocate fail\n");
    }

    if(1 != fread(buffer, lSize, 1, fp)) {
        fclose(fp);
        free(buffer);
        print_error("ERROR entire read fails");
    }

    fclose(fp);

    // init variables
    int index;

    // DNS translate
    server = gethostbyname(hostname);

    // build server address
    bzero((char *) &serveraddr[0], sizeof(struct sockaddr_in));
    serveraddr[0].sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr[0].sin_addr.s_addr, server->h_length);
    serveraddr[0].sin_port = htons(port);

    // create socket
    if((sockfd[0] = mp_socket(AF_INET, SOCK_MPTCP, 0)) < 0)
        print_error("ERROR opening socket\n");

    // connect server
    if (mp_connect(sockfd[0], (struct sockaddr *)&serveraddr[0], sizeof(struct sockaddr_in)) < 0) 
        print_error("ERROR connecting\n");

    // request subflow connections
    bzero(buf[0], MSS);
    sprintf(buf[0], "MPREQ %d", n_paths);

    bzero((char *) &clientaddr[0], sizeof(struct sockaddr_in));
    bzero((char *) &hdr[0], sizeof(struct mptcp_header));
    bzero((char *) &pkt[0], sizeof(struct packet));
    
    if(getsockname(sockfd[0], (struct sockaddr *)&clientaddr[0], &client_len[0]) < 0)
        print_error("ERROR getting client address");
    
    hdr[0].dest_addr = serveraddr[0];
    hdr[0].src_addr = clientaddr[0];
    hdr[0].seq_num = 1;
    hdr[0].ack_num = 0;
    hdr[0].total_bytes = lSize;

    pkt[0].header = &hdr[0];
    pkt[0].data = buf[0];

    if(mp_send(sockfd[0], (struct packet *)&pkt[0], strlen(buf[0]), 0) != strlen(buf[0]))
        print_error("ERROR requesting subflow connections\n");

    // receive aviable ports
    bzero(buf[0], MSS);
    bzero((char *) &hdr[0], sizeof(struct mptcp_header));
    bzero((char *) &pkt[0], sizeof(struct packet));

    pkt[0].header = &hdr[0];
    pkt[0].data = buf[0];

    recv_len = mp_recv(sockfd[0], (struct packet *)&pkt[0], MSS, 0);
    if(recv_len < 0)
        print_error("ERROR receiving packet failure");

    // create multipath connection
    bzero(buf[1], MSS);
    strncpy(buf[1], pkt[0].data + 5, strlen(pkt[0].data) - 5);
    close(sockfd[0]);

    token = strtok(buf[1], ":");

    index = 0;
    while(token != NULL) {
        // build server address
        bzero((char *) &serveraddr[index], sizeof(struct sockaddr_in));
        serveraddr[index].sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serveraddr[index].sin_addr.s_addr, server->h_length);
        serveraddr[index].sin_port = htons(atoi(token));

        // create sockets
        if((sockfd[index] = mp_socket(AF_INET, SOCK_MPTCP, 0)) < 0)
            print_error("ERROR opening socket\n");

        // connect paths
        if (mp_connect(sockfd[index], (struct sockaddr *)&serveraddr[index], sizeof(struct sockaddr_in)) < 0)
            print_error("ERROR connecting with path");
        
        token = strtok(NULL, ":");
        index++;
    }

    // file sender management
    offset = 0;
    ack = 0;

    for(index = 0; index < n_paths; index++) {
        if(pthread_create(&th[index], NULL, write_thread_mptcp, &sockfd[index]))
            print_error("ERROR creating pthread");
    }
    if(pthread_create(&th[16], NULL, read_thread_mptcp, NULL))
        print_error("ERROR creating pthread");
    if(pthread_join(th[16], NULL))
        print_error("ERROR joing pthread recv");

    // close all connection & free memory
    free(buffer);

    for(index = 0; index < n_paths; index++)
        close(sockfd[index]);

    // time report
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    double speed = lSize / time_spent;
    printf("====================ALL TRANSMISSION DONE=========================\n");
    printf("%-50s", "Total file transmission size:");
    printf("%lu bytes\n", lSize);
    printf("%-50s", "Time spend:");
    printf("%f seconds\n", time_spent);
    printf("%-50s", "Average transmission speed:");
    printf("%f B/s\n", speed);
    return 0;
}