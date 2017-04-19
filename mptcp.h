#ifndef MPTCP_H
#define MPTCP_H

// C libraries
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

// MPTCP macros
#define SOCK_MPTCP 2          // socket type for mptcp connections
#define RWIN       2048       // maximum receiver window
#define MSS        128        // sender maximum segment size

/******************************************************************************
 * TCP data structures                                                        *
 ******************************************************************************/
/******************************************************************************
 * packet header holds information about the current packet + connection      *
 ******************************************************************************/
struct mptcp_header {
    struct sockaddr_in dest_addr;    // remote destination address
    struct sockaddr_in src_addr;     // local sender address
    int                seq_num;      // sequence number ( first data byte )
    int                ack_num;      // ACK number ( next expected data byte )
    int                total_bytes;  // total bytes of data to transmit
};

/******************************************************************************
 * packet holds information about the data contained + connection             *
 ******************************************************************************/
struct packet {
    struct mptcp_header * header;    // pointer to packet header
    char                * data;      // segment of data
};


/******************************************************************************
 * TCP wrapper functions                                                      *
 ******************************************************************************/
/******************************************************************************
 * receive UDP datagram(s) as TCP packet                                      *
 *  - returns total bytes of data received                                    *
 ******************************************************************************/
ssize_t mp_recv   ( int sockfd, struct packet * recv_pkt, size_t data_len, int flags );

/******************************************************************************
 * send UDP datagram(s) as TCP packet                                         *
 *  - returns total bytes of data sent                                        *
 ******************************************************************************/
ssize_t mp_send   ( int sockfd, const struct packet * send_pkt, size_t data_len, int flags );

/******************************************************************************
 * create connection with hostname, port                                      *
 *  - returns 0 on success                                                    *
 ******************************************************************************/
int     mp_connect( int sockfd, const struct sockaddr * addr, socklen_t addrlen );

/******************************************************************************
 * create MPTCP socket                                                        *
 *  - returns socket descriptor for new connection                            *
 ******************************************************************************/
int     mp_socket ( int domain, int type, int protocol );

/******************************************************************************
 * prints packet header information + data load                               *
 *  - returns nothing, prints to standard out                                 *
 ******************************************************************************/
void    print_pkt( const struct packet * );

#endif // MPTCP_H