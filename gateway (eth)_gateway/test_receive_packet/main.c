/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include <pthread.h>

#define MY_MAC0	0xa0
#define MY_MAC1	0xf6
#define MY_MAC2	0xfd
#define MY_MAC3	0x64
#define MY_MAC4	0xaf
#define MY_MAC5	0xfc

#define VLC_MAC0 0xb0
#define VLC_MAC1 0xd5
#define VLC_MAC2 0xcc
#define VLC_MAC3 0xfc
#define VLC_MAC4 0xfe
#define VLC_MAC5 0x85

#define RF_MAC0 0x18
#define RF_MAC1 0xa6
#define RF_MAC2 0xf7
#define RF_MAC3 0x1a
#define RF_MAC4 0xa9
#define RF_MAC5 0x04

#define NODE_MAC0	0xec
#define NODE_MAC1	0x08
#define NODE_MAC2	0x6b
#define NODE_MAC3	0x13
#define NODE_MAC4	0x1e
#define NODE_MAC5	0x5c

#define ETHER_TYPE	0x0800

#define ETH_IF_NAME	"eth0"
#define RF_IF_NAME	"ra0"
#define BUF_SIZ		1024

#define N_VLC 1

unsigned char ** vlc_addresses;

int eth_fd;
struct ifreq eth_if_mac;	//Mac Address of the Ethernet Interface

int rf_fd;
struct ifreq rf_if_idx;
struct ifreq rf_if_mac;	//Mac Address of the RF Interface

void setup_vlc_address(){
	vlc_addresses = (uint8_t **) malloc(N_VLC * sizeof(uint8_t*));

	//TODO: FILL THE MAC ADDRESS OF VLC TXs
	vlc_addresses[0] = (uint8_t *) malloc(6 * sizeof(uint8_t));
	vlc_addresses[0][0] = 0xb0;
	vlc_addresses[0][1] = 0xd5;
	vlc_addresses[0][2] = 0xcc;
	vlc_addresses[0][3] = 0xfc;
	vlc_addresses[0][4] = 0xfe;
	vlc_addresses[0][5] = 0x85;
}

int is_vlc_tx_address(struct ether_header *eh){
	int is_valid_vlc_address = 0;
	int i;

	for(i = 0;i <N_VLC;i++){
		 is_valid_vlc_address = (eh->ether_shost[0] == vlc_addresses[0][0] &&
				 	 	 	 	 eh->ether_shost[1] == vlc_addresses[0][1] &&
								 eh->ether_shost[2] == vlc_addresses[0][2] &&
								 eh->ether_shost[3] == vlc_addresses[0][3] &&
								 eh->ether_shost[4] == vlc_addresses[0][4] &&
								 eh->ether_shost[5] == vlc_addresses[0][5] &&
								 eh->ether_dhost[0] == MY_MAC0 &&
								 eh->ether_dhost[1] == MY_MAC1 &&
								 eh->ether_dhost[2] == MY_MAC2 &&
								 eh->ether_dhost[3] == MY_MAC3 &&
								 eh->ether_dhost[4] == MY_MAC4 &&
								 eh->ether_dhost[5] == MY_MAC5);
		 if(is_valid_vlc_address){
			 break;
		 }
	}

	return is_valid_vlc_address;
}

int is_from_node(struct ether_header *eh){
//	printf("Sender Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	return (eh->ether_shost[0] == NODE_MAC0 &&
			 eh->ether_shost[1] == NODE_MAC1 &&
			 eh->ether_shost[2] == NODE_MAC2 &&
			 eh->ether_shost[3] == NODE_MAC3 &&
			 eh->ether_shost[4] == NODE_MAC4 &&
			 eh->ether_shost[5] == NODE_MAC5 &&
			 eh->ether_dhost[0] == RF_MAC0 &&
			 eh->ether_dhost[1] == RF_MAC1 &&
			 eh->ether_dhost[2] == RF_MAC2 &&
			 eh->ether_dhost[3] == RF_MAC3 &&
			 eh->ether_dhost[4] == RF_MAC4 &&
			 eh->ether_dhost[5] == RF_MAC5);
//	int is_valid_vlc_address = 0;
//	int i;
//
//	for(i = 0;i <N_VLC;i++){
//		 is_valid_vlc_address = (eh->ether_shost[0] == vlc_addresses[0][0] &&
//				 	 	 	 	 eh->ether_shost[1] == vlc_addresses[0][1] &&
//								 eh->ether_shost[2] == vlc_addresses[0][2] &&
//								 eh->ether_shost[3] == vlc_addresses[0][3] &&
//								 eh->ether_shost[4] == vlc_addresses[0][4] &&
//								 eh->ether_shost[5] == vlc_addresses[0][5] &&
//								 eh->ether_dhost[0] == MY_MAC0 &&
//								 eh->ether_dhost[1] == MY_MAC1 &&
//								 eh->ether_dhost[2] == MY_MAC2 &&
//								 eh->ether_dhost[3] == MY_MAC3 &&
//								 eh->ether_dhost[4] == MY_MAC4 &&
//								 eh->ether_dhost[5] == MY_MAC5);
//		 if(is_valid_vlc_address){
//			 break;
//		 }
//	}
//
//	return is_valid_vlc_address;
}

int setup_vlc_socket(){
	int sockopt;
	char eth_if_name[IFNAMSIZ]; //Name of Ethernet Interface
	strcpy(eth_if_name, ETH_IF_NAME);

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((eth_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("listener: socket");
		return -1;
	}

	memset(&eth_if_mac, 0, sizeof(struct ifreq));
	strncpy(eth_if_mac.ifr_name, eth_if_name, IFNAMSIZ-1);
	if (ioctl(eth_fd, SIOCGIFHWADDR, &eth_if_mac) < 0)
		perror("SIOCGIFHWADDR");

	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(eth_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(eth_fd);
//		exit(EXIT_FAILURE);
		return -1;
	}
	/* Bind to device */
	if (setsockopt(eth_fd, SOL_SOCKET, SO_BINDTODEVICE, eth_if_name, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(eth_fd);
//		exit(EXIT_FAILURE);
		return -1;
	}

	return 0;
}

void *transmit_packet_vlc(void *thread_id){
	int eth_bytes_sent;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[5];
	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
	eh->ether_dhost[0] = VLC_MAC0;
	eh->ether_dhost[1] = VLC_MAC1;
	eh->ether_dhost[2] = VLC_MAC2;
	eh->ether_dhost[3] = VLC_MAC3;
	eh->ether_dhost[4] = VLC_MAC4;
	eh->ether_dhost[5] = VLC_MAC5;
	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ALL);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	sendbuf[tx_len++] = 0xde;
	sendbuf[tx_len++] = 0xad;
	sendbuf[tx_len++] = 0xbe;
	sendbuf[tx_len++] = 0xef;

	/* Index of the network device */
//	socket_address.sll_ifindex = eth_if_idx.ifr_ifindex;
	socket_address.sll_ifindex = 2;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = VLC_MAC0;
	socket_address.sll_addr[1] = VLC_MAC1;
	socket_address.sll_addr[2] = VLC_MAC2;
	socket_address.sll_addr[3] = VLC_MAC3;
	socket_address.sll_addr[4] = VLC_MAC4;
	socket_address.sll_addr[5] = VLC_MAC5;

	/* Send packet */
	for(;;){
		eth_bytes_sent = sendto(eth_fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
		if (eth_bytes_sent < 0){
			printf("Send failed\n");
		}

//		printf("%d bytes sent from eth\n",eth_bytes_sent);
	}

	pthread_exit(NULL);
}

void *receive_vlc_report(void *thread_id){
	int i;

	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
	struct ether_header *eh = (struct ether_header *) buf;

	for(;;){
		numbytes = recvfrom(eth_fd, buf, BUF_SIZ, 0, NULL, NULL);
//		printf("listener: got packet %lu bytes\n", numbytes);

		/* Check the packet is for me */
		if (is_vlc_tx_address(eh)) {
//			printf("Correct destination MAC address\n");
//			printf("\tData:");
//			for (i=0; i<numbytes; i++) printf("%02x:", buf[i]);
//			printf("\n");
			printf("received %d numbytes from VLC TX\n",numbytes);

			report_content = (unsigned char*) (buf + sizeof(struct ether_header));
			for (i=0; i<4; i++) printf("%02x:", report_content[i]);
						printf("\n");

		}
	}

	pthread_exit(NULL);
}

int setup_rf_socket(){
	int sockopt;
	char rf_if_name[IFNAMSIZ]; //Name of Ethernet Interface
	strcpy(rf_if_name, RF_IF_NAME);

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((rf_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("listener: socket");
		return -1;
	}

	memset(&rf_if_mac, 0, sizeof(struct ifreq));
	strncpy(rf_if_mac.ifr_name, rf_if_name, IFNAMSIZ-1);
	if (ioctl(rf_fd, SIOCGIFHWADDR, &rf_if_mac) < 0)
		perror("SIOCGIFHWADDR");

	memset(&rf_if_idx, 0, sizeof(struct ifreq));
	strncpy(rf_if_idx.ifr_name, rf_if_name, IFNAMSIZ-1);
	if(ioctl(rf_fd,SIOCGIFINDEX,&rf_if_idx) == -1){
//		printf("%s", strerror(errno));
		perror("SIOCGIFINDEX");
		return -1;
	}

	printf("IF IDX(RF): %d\n",rf_if_idx.ifr_ifindex);

	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(rf_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(rf_fd);
//		exit(EXIT_FAILURE);
		return -1;
	}
	/* Bind to device */
	if (setsockopt(rf_fd, SOL_SOCKET, SO_BINDTODEVICE, rf_if_name, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(rf_fd);
//		exit(EXIT_FAILURE);
		return -1;
	}

	return 0;
}

void *transmit_packet_rf(void *thread_id){
	int rf_bytes_sent;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[5];
	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
	eh->ether_dhost[0] = NODE_MAC0;
	eh->ether_dhost[1] = NODE_MAC1;
	eh->ether_dhost[2] = NODE_MAC2;
	eh->ether_dhost[3] = NODE_MAC3;
	eh->ether_dhost[4] = NODE_MAC4;
	eh->ether_dhost[5] = NODE_MAC5;
	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ALL);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	sendbuf[tx_len++] = 0xde;
	sendbuf[tx_len++] = 0xad;
	sendbuf[tx_len++] = 0xbe;
	sendbuf[tx_len++] = 0xef;

	/* Index of the network device */
//	socket_address.sll_ifindex = eth_if_idx.ifr_ifindex;
	socket_address.sll_ifindex = 2;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = NODE_MAC0;
	socket_address.sll_addr[1] = NODE_MAC1;
	socket_address.sll_addr[2] = NODE_MAC2;
	socket_address.sll_addr[3] = NODE_MAC3;
	socket_address.sll_addr[4] = NODE_MAC4;
	socket_address.sll_addr[5] = NODE_MAC5;

	/* Send packet */
	for(;;){
		rf_bytes_sent = sendto(rf_fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
		if (rf_bytes_sent < 0){
			printf("Send failed\n");
		}

//		printf("%d bytes sent from rf\n",rf_bytes_sent);
	}

	pthread_exit(NULL);
}

void *receive_node_report(void *thread_id){
	int i;

	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
	struct ether_header *eh = (struct ether_header *) buf;

	for(;;){
		numbytes = recvfrom(rf_fd, buf, BUF_SIZ, 0, NULL, NULL);
//		printf("listener: got packet %lu bytes\n", numbytes);

		/* Check the packet is for me */
//		if(numbytes == 64){
			if (is_from_node(eh)) {
	//			printf("Correct destination MAC address\n");
	//			printf("\tData:");
	//			for (i=0; i<numbytes; i++) printf("%02x:", buf[i]);
	//			printf("\n");
				printf("received %d numbytes from VLC TX\n",numbytes);

				report_content = (unsigned char*) (buf + sizeof(struct ether_header));
				for (i=0; i<4; i++) printf("%02x:", report_content[i]);
							printf("\n");

			}
//		}
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	setup_vlc_address();
	setup_vlc_socket();
	setup_rf_socket();


	/* Header structures */
	pthread_t threads[4];
	if(pthread_create(&threads[0],NULL,receive_vlc_report,(void *)0)
			|| pthread_create(&threads[1],NULL,transmit_packet_vlc,(void *)1)
			|| pthread_create(&threads[2],NULL,transmit_packet_rf,(void *)2)
			|| pthread_create(&threads[3],NULL,receive_node_report,(void *)3)
			){
		printf("Error initializing thread\n");
		exit(-1);
	}

	for(;;){

	}

	close(eth_fd);
	return 0;
}
