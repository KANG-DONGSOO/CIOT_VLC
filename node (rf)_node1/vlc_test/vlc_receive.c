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

#define INTERFACE "vlc0"
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
			printf("data received\n");
		}
	}

	close(fd);
	return ret;
}
