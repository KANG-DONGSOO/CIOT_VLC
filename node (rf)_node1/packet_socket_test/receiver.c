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

#define DEST_MAC0 0xa0
#define DEST_MAC1 0xf6
#define DEST_MAC2 0xfd
#define DEST_MAC3 0x87
#define DEST_MAC4 0x9a
#define DEST_MAC5 0x5e

#define INTERFACE "eth0"
#define BUF_SIZE 1024

int main(int argc, char * argv){

	const unsigned char ether_broadcast_addr[] = {
			0xff,0xff,0xff,0xff,0xff,0xff
	};
	char sender[INET6_ADDRSTRLEN];
	int fd, ret, i,if_name_len,if_idx;
	struct packet_mreq mr;
	int sockopt;
	ssize_t numbytes = 0;
	struct ifreq ifr;
	struct sockaddr_ll addr = {0};
	struct sockaddr_storage their_addr;
	uint8_t buff[BUF_SIZE];
	char *if_name;
	struct ether_header *eh;
	unsigned char* packet_content;
	int is_repeat = 1;
	socklen_t fromlen;

	if_name = INTERFACE;
	eh = (struct ether_header *) buff;
	packet_content = (char *) (buff + sizeof(struct ether_header));


	//Domain Packet Socket, Socket type Raw (define header manually), Protocol for experimental
	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if(fd == -1){
		printf("error socket");
	}

	//Determine the Index number of interface used
	if_name_len = strlen(if_name);
	if(if_name_len < sizeof(ifr.ifr_name)){
		memcpy(ifr.ifr_name, if_name, if_name_len);
		ifr.ifr_name[if_name_len] = 0;
	}
	else{
		printf("interface name too long");
	}

	if(ioctl(fd,SIOCGIFINDEX,&ifr) == -1){
		printf("fail to get index of interface");
	}

	if_idx = ifr.ifr_ifindex;
	printf("IF Index: %d\n", if_idx);

	//Construct Destination Address
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = if_idx;
	addr.sll_halen = ETHER_ADDR_LEN;
	addr.sll_protocol = htons(ETH_P_ALL);
	memcpy(addr.sll_addr,ether_broadcast_addr,ETHER_ADDR_LEN);

	if(bind(fd,(struct sockaddr*)&addr, sizeof(addr))<0){
		printf("error binding\n");
	}
	fromlen = sizeof(struct sockaddr_ll);

	//Set the option of the socket
	memset(&mr,0,sizeof(mr));
	mr.mr_ifindex = ifr.ifr_ifindex;
	mr.mr_type = PACKET_MR_PROMISC; //TODO: Dont know what type is the correct one
	setsockopt(fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr));

	printf("listener: Waiting to recvfrom...\n");
	while(is_repeat){
		numbytes = recvfrom(fd,buff,BUF_SIZE,0,(struct sockaddr*)&addr, &fromlen);
		printf("%d\n",numbytes);
		if(numbytes>0){
			//Check if the packet is intended to this node
			printf("%x:%x:%x:%x:%x:%x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
			if(eh->ether_dhost[0] == DEST_MAC0 &&
					eh->ether_dhost[1] == DEST_MAC1 &&
					eh->ether_dhost[2] == DEST_MAC2 &&
					eh->ether_dhost[3] == DEST_MAC3 &&
					eh->ether_dhost[4] == DEST_MAC4 &&
					eh->ether_dhost[5] == DEST_MAC5){

					printf("listener: got packet %lu bytes\n",numbytes);
					printf("ethernet header: %d \n",sizeof(struct ether_header));
//				printf("Packet Received\ndata:");
//				for(i=0;i<numbytes;i++){
//					printf("%02x",buff[i]);
//				}
//				printf("\n");
				ret = 0;
				is_repeat = 0;
			} else{
				ret = -1;
				//printf("wrong destination");
			}
		}
	}


	close(fd);
	return ret;
}
