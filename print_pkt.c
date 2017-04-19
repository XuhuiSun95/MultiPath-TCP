#include <arpa/inet.h>
#include "mptcp.h"

/******************************************************************************
 * prints packet header information + data load                               *
 *  - returns nothing, prints to standard out                                 *
 ******************************************************************************/
void print_pkt( const struct packet * pkt ) {
    printf( "dest port   : %d\n", ntohs(pkt->header->dest_addr.sin_port) );
    printf( "dest addr   : %s\n", inet_ntoa( pkt->header->dest_addr.sin_addr ) );
    printf( "src port    : %d\n", ntohs(pkt->header->src_addr.sin_port) );
    printf( "src addr    : %s\n", inet_ntoa( pkt->header->src_addr.sin_addr ) );
    printf( "seq_num     : %d\n", pkt->header->seq_num );
    printf( "ack_num     : %d\n", pkt->header->ack_num );
    printf( "total_bytes : %d\n", pkt->header->total_bytes );
    printf( "data        : %s\n", pkt->data );
}
