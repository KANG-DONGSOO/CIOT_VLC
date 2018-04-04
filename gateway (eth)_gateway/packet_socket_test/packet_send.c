#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define INTERFACE_NAME "eth0"
#define BUF_SIZE 1024

#define DEST_MAC0 0xa0
#define DEST_MAC1 0xf6
#define DEST_MAC2 0xfd
#define DEST_MAC3 0x87
#define DEST_MAC4 0x9a
#define DEST_MAC5 0x5e

int main(int argc, char *argv[]){

	const unsigned char ether_broadcast_addr[] = {
			0xff,0xff,0xff,0xff,0xff,0xff
	};
	char * if_name = INTERFACE_NAME;
	struct ifreq if_idx,if_mac;
	size_t if_name_len;
	int len = 0;
	struct sockaddr_ll addr = {0};
	struct packet_mreq mr;
	char buff[BUF_SIZE];
	struct ether_header *eth;



}
