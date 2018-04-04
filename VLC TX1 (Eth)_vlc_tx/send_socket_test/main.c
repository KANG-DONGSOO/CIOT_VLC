/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include <pthread.h>

#define GATEWAY_DEST_MAC0	0xa0
#define GATEWAY_DEST_MAC1	0xf6
#define GATEWAY_DEST_MAC2	0xfd
#define GATEWAY_DEST_MAC3	0x64
#define GATEWAY_DEST_MAC4	0xaf
#define GATEWAY_DEST_MAC5	0xfc

#define MY_ETH_MAC0 0xb0
#define MY_ETH_MAC1 0xd5
#define MY_ETH_MAC2 0xcc
#define MY_ETH_MAC3 0xfc
#define MY_ETH_MAC4 0xfe
#define MY_ETH_MAC5 0x85

#define ETH_IF_NAME	"eth0"
#define GATEWAY_ETH_IF_IDX 2
#define BUF_SIZ		1024

int eth_fd;				//File Descriptor for Ethernet Socket
struct ifreq eth_if_mac;	//Mac Address of the

int setup_eth_socket(){
	int sockopt;
	struct ifreq eth_if_idx;	//Index of the interface the we use to send from
	char if_name[IFNAMSIZ];	//Interface Name
	/* Get interface name */
	strcpy(if_name, ETH_IF_NAME);

	/* Open RAW socket to send on */
	if ((eth_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("socket");
		return -1;
	}

	/* Get the index of the interface to send on */
	memset(&eth_if_idx, 0, sizeof(struct ifreq));
	strncpy(eth_if_idx.ifr_name, if_name, IFNAMSIZ-1);
	if (ioctl(eth_fd, SIOCGIFINDEX, &eth_if_idx) < 0)
		perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&eth_if_mac, 0, sizeof(struct ifreq));
	strncpy(eth_if_mac.ifr_name, if_name, IFNAMSIZ-1);
	if (ioctl(eth_fd, SIOCGIFHWADDR, &eth_if_mac) < 0)
		perror("SIOCGIFHWADDR");

	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(eth_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(eth_fd);
		exit(EXIT_FAILURE);
	}

	/* Bind to device */
	if (setsockopt(eth_fd, SOL_SOCKET, SO_BINDTODEVICE, if_name, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(eth_fd);
//		exit(EXIT_FAILURE);
		return -1;
	}
}

int is_for_this_tx(struct ether_header *eh){
	return (eh->ether_shost[0] == GATEWAY_DEST_MAC0 &&
 	 	 	 eh->ether_shost[1] == GATEWAY_DEST_MAC1 &&
			 eh->ether_shost[2] == GATEWAY_DEST_MAC2 &&
			 eh->ether_shost[3] == GATEWAY_DEST_MAC3 &&
			 eh->ether_shost[4] == GATEWAY_DEST_MAC4 &&
			 eh->ether_shost[5] == GATEWAY_DEST_MAC5 &&
			 eh->ether_dhost[0] == MY_ETH_MAC0 &&
			 eh->ether_dhost[1] == MY_ETH_MAC1 &&
			 eh->ether_dhost[2] == MY_ETH_MAC2 &&
			 eh->ether_dhost[3] == MY_ETH_MAC3 &&
			 eh->ether_dhost[4] == MY_ETH_MAC4 &&
			 eh->ether_dhost[5] == MY_ETH_MAC5);
}

void *send_vlc_report(void *thread_id){
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
	eh->ether_dhost[0] = GATEWAY_DEST_MAC0;
	eh->ether_dhost[1] = GATEWAY_DEST_MAC1;
	eh->ether_dhost[2] = GATEWAY_DEST_MAC2;
	eh->ether_dhost[3] = GATEWAY_DEST_MAC3;
	eh->ether_dhost[4] = GATEWAY_DEST_MAC4;
	eh->ether_dhost[5] = GATEWAY_DEST_MAC5;
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
	socket_address.sll_addr[0] = GATEWAY_DEST_MAC0;
	socket_address.sll_addr[1] = GATEWAY_DEST_MAC1;
	socket_address.sll_addr[2] = GATEWAY_DEST_MAC2;
	socket_address.sll_addr[3] = GATEWAY_DEST_MAC3;
	socket_address.sll_addr[4] = GATEWAY_DEST_MAC4;
	socket_address.sll_addr[5] = GATEWAY_DEST_MAC5;

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
		if (is_for_this_tx(eh)) {
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

int main(int argc, char *argv[])
{
	setup_eth_socket();

	/* Header structures */
	pthread_t threads[4];
	if(pthread_create(&threads[0],NULL,send_vlc_report,(void *)0)
			|| pthread_create(&threads[1],NULL,receive_vlc_report,(void *)1)

			){
		printf("Error initializing thread\n");
		exit(-1);
	}

	for(;;){

	}

	close(eth_fd);
	return 0;
}
