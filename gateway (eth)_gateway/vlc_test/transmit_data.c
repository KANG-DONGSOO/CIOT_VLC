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

#define INTERFACE_NAME "vlc0"
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


	//Domain Packet Socket, Socket type Raw (define header manually), Protocol all
	int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if(fd == -1){
		printf("%s", strerror(errno));
	}

	//Determine the Index number of interface used
	if_name_len = strlen(if_name);
	if(if_name_len < sizeof(if_idx.ifr_name)){
		memcpy(if_idx.ifr_name, if_name, if_name_len);
		if_idx.ifr_name[if_name_len] = 0;
	}
	else{
		printf("interface name too long");
	}

	if(ioctl(fd,SIOCGIFINDEX,&if_idx) == -1){
		printf("%s", strerror(errno));
	}

	printf("IF Index: %d\n", if_idx.ifr_ifindex);

	//Determine the MAC Address of interface to send on
//	if(if_name_len < sizeof(if_mac.ifr_name)){
//		memcpy(if_mac.ifr_name, if_name, if_name_len);
//		if_mac.ifr_name[if_name_len] = 0;
//	}
//	else{
//		printf("interface name too long");
//	}
//
//	if(ioctl(fd,SIOCGIFHWADDR, &if_mac) <0){
//		printf("%s", strerror(errno));
//	}


	//Construct Destination Address
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = if_idx.ifr_ifindex;
	addr.sll_protocol = htons(ETH_P_ALL);
	bind(fd,(struct sockaddr*)&addr, sizeof(addr));

	//Set the option of the socket
	memset(&mr,0,sizeof(mr));
	mr.mr_ifindex = if_idx.ifr_ifindex;
	mr.mr_type = PACKET_MR_PROMISC; //TODO: Dont know what type is the correct one
	setsockopt(fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr));


	//Frame construction
	eth = (struct ether_header*) buff;
	memset(buff,0,BUF_SIZE);
	eth->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eth->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eth->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eth->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eth->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eth->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eth->ether_dhost[0] = DEST_MAC0;
	eth->ether_dhost[1] = DEST_MAC1;
	eth->ether_dhost[2] = DEST_MAC2;
	eth->ether_dhost[3] = DEST_MAC3;
	eth->ether_dhost[4] = DEST_MAC4;
	eth->ether_dhost[5] = DEST_MAC5;
	eth->ether_type = htons(ETH_P_ALL);
	len+=sizeof(struct ether_header);

	//Content from here
	buff[len++] = (unsigned char) 'd';
	buff[len++] = (unsigned char) 'u';
	buff[len++] = (unsigned char) 'm';
	buff[len++] = (unsigned char) 'm';
	buff[len++] = (unsigned char) 'y';
	buff[len++] = (unsigned char) '89';
	buff[len++] = (unsigned char) '\0';

	int writtenbytes = write(fd,buff,len);
	if(writtenbytes < 0){
		printf("%s\n", strerror(errno));
	}
	else{
		printf("written %d bytes\n",writtenbytes);
	}

	return 0;
}
