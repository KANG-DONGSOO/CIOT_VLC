#include "vlctx.h"

//Ethernet Socket variables
int eth_fd;				//File Descriptor for Ethernet Socket
struct ifreq eth_if_mac;	//Mac Address of the

//VLC Socket variables
uint8_t vlc_buff[BUF_SIZ];
int vlc_fd;
struct ether_header *eth;

//Packet Queue Variables
Queue *vlc_queue;
int queue_size = 0;

uint8_t gateway_address[] = {
		0xa0,0xf6,0xfd,0x64,0xaf,0xfc
};

void *transmit_vlc(void *thread_id){
	struct gateway_packet vlc_packet;

	for(;;){
		if(!isEmpty(vlc_queue)){
			//Check from packet Queue, if not empty, Pop and Transmit
//			printf("Queue not empty, pop from queue and send to VLC\n");
			NODE *from_queue;
			from_queue = Dequeue(vlc_queue);
			if(from_queue != NULL){
				vlc_packet = from_queue->data;
				free(from_queue);

				transmit_to_vlc(vlc_packet);
				queue_size--;
				printf("transmit to node\n"); //[DEBUG]Checking if transmit_to_vlc function ever returns

				free(vlc_packet.data);
				usleep(SCHEDULING_PERIOD);
			}
		}
	}

	pthread_exit(NULL);
}

void *send_queue_report(void *thread_id){
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
//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
	eh->ether_dhost[0] = GATEWAY_DEST_MAC0;
	eh->ether_dhost[1] = GATEWAY_DEST_MAC1;
	eh->ether_dhost[2] = GATEWAY_DEST_MAC2;
	eh->ether_dhost[3] = GATEWAY_DEST_MAC3;
	eh->ether_dhost[4] = GATEWAY_DEST_MAC4;
	eh->ether_dhost[5] = GATEWAY_DEST_MAC5;
//	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ALL);
//	tx_len += sizeof(struct ether_header);

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
		tx_len = sizeof(struct ether_header);
		sendbuf[tx_len++] = (unsigned char) (SELF_ID & 0xff);
		memcpy(&sendbuf[tx_len],(unsigned char*)&queue_size,sizeof(int));
//		printf("queue size of %d is reported\n",queue_size);
		tx_len+=sizeof(int);

		eth_bytes_sent = sendto(eth_fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
		if (eth_bytes_sent < 0){
			printf("Send failed\n");
		}

//		printf("%d bytes sent from eth\n",eth_bytes_sent);
		usleep(REPORT_PERIOD);
	}

	pthread_exit(NULL);
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

int main(int argc,char* argv){

	int i;
	unsigned char* packet_content;
	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) buf;

	//INITIALIZATION
	setup_eth_socket();
	setup_vlc_socket();

	vlc_queue = ConstructQueue(QUEUE_LIMIT);

	//Thread FOR QUEUE SIZE REPORTING AND VLC TRANSMISSION
	pthread_t threads[2];
	if(pthread_create(&threads[VLC_TRANSMIT_THREAD_ID],NULL,transmit_vlc,(void *)VLC_TRANSMIT_THREAD_ID)
		||pthread_create(&threads[QUEUE_REPORT_THREAD_ID],NULL,send_queue_report,(void *)QUEUE_REPORT_THREAD_ID)){

		printf("Error initializing thread\n");
		exit(-1);
	}


	printf("--INITIALIZATION FINISHED--\n");
	for(;;){

		numbytes = recvfrom(eth_fd, buf, BUF_SIZ, 0, NULL, NULL);
//		printf("listener: got packet %lu bytes\n", numbytes);

		/* Check the packet is for me */
		if (is_for_this_tx(eh)) {
				packet_content = (unsigned char *) (buf + sizeof(struct ether_header));
//				printf("Received packet:\n");
//				for(i =0; i<14;i++){
//					printf("0x%02x ",packet_content[i]);
//				}
//				printf("-----------\n");

				NODE *temp = (NODE *) malloc(sizeof(NODE));
				int i;
				struct gateway_packet buffer_content = generate_gateway_packet_from_bytes(packet_content);

				temp->data.node_id = buffer_content.node_id;
				temp->data.vlc_id = buffer_content.vlc_id;
				temp->data.payload_len = buffer_content.payload_len;
//				printf("Payload len: %d\n",buffer_content.payload_len);
				temp->data.seq_number = buffer_content.seq_number;
				printf("Seq Number: %d\n",buffer_content.seq_number);
				temp->data.data = (uint8_t *) malloc(buffer_content.payload_len * sizeof(uint8_t));
				for(i=0;i<buffer_content.payload_len;i++){
					temp->data.data[i] = buffer_content.data[i];
				}

				if((queue_size+1) < QUEUE_LIMIT){	//IF the number of packets in queue already on the limit, the packets will be dropped
					Enqueue(vlc_queue,temp);
					printf("Added a packet for NODE-%d\n",buffer_content.node_id);
					queue_size++;
				}
		}

	}

	return 0;
}

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


	return 1;
}

int transmit_to_vlc(struct gateway_packet packet){
	int len = 0,i,my_id;

	my_id = SELF_ID;
	//Content from here
	memcpy(&vlc_buff[len],(uint8_t*)&packet.payload_len,sizeof(uint16_t));
	len+=sizeof(uint16_t);
	memcpy(&vlc_buff[len],(uint8_t*)&packet.node_id,sizeof(uint16_t));
	len+=sizeof(uint16_t);
	memcpy(&vlc_buff[len],(uint8_t*)&packet.vlc_id,sizeof(uint16_t));
	len+=sizeof(uint16_t);
	memcpy(&vlc_buff[len],(uint8_t*)&packet.seq_number,sizeof(uint32_t));
	len+=sizeof(uint32_t);
	for(i=0;i<packet.payload_len;i++){
		vlc_buff[len] = packet.data[i];
		len+=sizeof(uint8_t);
	}

//	for(i=0;i<len;i++){
//		printf("0x%02x ",vlc_buff[i]);
//	}
//	printf("\n");

	while(write(vlc_fd,vlc_buff,len) < 0){
		//printf("%s\n", strerror(errno));
		if(errno ==ENOBUFS){
			usleep(100);
		}
	}

	return 1;
}

int setup_vlc_socket(){
	const unsigned char ether_broadcast_addr[] = {
			0xff,0xff,0xff,0xff,0xff,0xff
	};
	char * if_name = VLC_INTERFACE;
	struct ifreq if_idx;
	size_t if_name_len;

	struct sockaddr_ll vlc_addr = {0};
	struct packet_mreq mr;


	//Domain Packet Socket, Socket type Raw (define header manually), Protocol all
	vlc_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if(vlc_fd == -1){
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

	if(ioctl(vlc_fd,SIOCGIFINDEX,&if_idx) == -1){
		printf("%s", strerror(errno));
	}

	printf("VLC IF Index: %d\n", if_idx.ifr_ifindex);

	//Construct Destination Address
	vlc_addr.sll_family = AF_PACKET;
	vlc_addr.sll_ifindex = if_idx.ifr_ifindex;
	vlc_addr.sll_protocol = htons(ETH_P_ALL);

	bind(vlc_fd,(struct sockaddr*)&vlc_addr, sizeof(vlc_addr));

	//Set the option of the socket
//	memset(&mr,0,sizeof(mr));
//	mr.mr_ifindex = if_idx.ifr_ifindex;
//	mr.mr_type = PACKET_MR_PROMISC; //TODO: Dont know what type is the correct one
//	setsockopt(vlc_fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr));
}

