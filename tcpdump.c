#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>

#include "battool.h"


void tcpdump_usage() {
	printf("Battool module tcpdump\n");
	printf("Usage: battool tcpdump|td interface\n");
	printf("\t-h help\n");
	return;
}

void print_batman_packet( unsigned char *buf)
{
	struct batman_packet *bp = (struct batman_packet *)buf;
	printf("batman header:\n");
	printf(" ptype=%d flags=%x ttl=%u orig=%s seq=%u gwflags=%x version=%u\n", bp->packet_type,bp->flags, bp->ttl, ether_ntoa((struct ether_addr*) bp->orig), ntohs(bp->seqno), bp->gwflags, bp->version );
}

void print_packet( int length, unsigned char *buf )
{
	int i = 0;
	printf("\n");
	for( ; i < length; i++ ) {
		if( i == 0 )
			printf("0000| ");

		if( i != 0 && i%8 == 0 )
			printf("  ");
		if( i != 0 && i%16 == 0 )
			printf("\n%04d| ", i/16*10);

		printf("%02x ", buf[i] );
	}
	printf("\n");
}

int tcpdump_main( int argc, char **argv )
{
	int  optchar,
		packetsize = 2000;

	unsigned char packet[packetsize];
	uint8_t found_args = 1;
	int32_t rawsock;
	ssize_t rec_length;

	struct ether_header *eth = (struct ether_header *) packet;
	struct ifreq req;
	struct sockaddr_ll addr;

	char *devicename;

	while ( ( optchar = getopt ( argc, argv, "h" ) ) != -1 ) {
		switch( optchar ) {
			case 'h':
				tcpdump_usage();
				exit(EXIT_SUCCESS);
				break;
			default:
				tcpdump_usage();
				exit(EXIT_FAILURE);
		}
	}

	if ( argc <= found_args ) {
		tcpdump_usage();
		exit(EXIT_FAILURE);
	}

	devicename = argv[found_args];

	if ( ( rawsock = socket(PF_PACKET,SOCK_RAW,htons( 0x0842 ) ) ) < 0 ) {
		printf("Error - can't create raw socket: %s\n", strerror(errno) );
		exit( EXIT_FAILURE );
	}

	strncpy(req.ifr_name, devicename, IFNAMSIZ);

	if ( ioctl(rawsock, SIOCGIFINDEX, &req) < 0 ) {
		printf("Error - can't create raw socket (SIOCGIFINDEX): %s\n", strerror(errno) );
		exit( EXIT_FAILURE );
	}

	addr.sll_family   = AF_PACKET;
	addr.sll_protocol = htons( 0x0842 );
	addr.sll_ifindex  = req.ifr_ifindex;

	if ( bind(rawsock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
		printf( "Error - can't bind raw socket: %s\n", strerror(errno) );
		close(rawsock);
		exit( EXIT_FAILURE );
	}

	while( ( rec_length = read(rawsock,packet,packetsize) ) > 0 ) {

		printf("\n---------------------------------------------------------------------------------------\n\n");
		printf("ethernet header:\n");
		printf(" %d bytes dest=%s ", rec_length, ether_ntoa( (struct ether_addr *)eth->ether_dhost ) );
		printf("src=%s type=%04x\n", ether_ntoa( (struct ether_addr *) eth->ether_shost ),  ntohs( eth->ether_type ) );

		if( packet[sizeof( struct ether_header)] == 0 )
			print_batman_packet( packet + sizeof( struct ether_header ) );
		else
			printf("message typ %d currently not supported\n", packet[sizeof( struct ether_header)] );

		print_packet( rec_length, packet );

	}

	exit( EXIT_SUCCESS );
}