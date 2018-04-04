#include "vlctx.h"

//Ethernet Socket variables
int eth_fd;				//File Descriptor for Ethernet Socket
struct ifreq eth_if_mac;	//Mac Address of the

int vlc_mutex_lock = 0;

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
		if(!isEmpty(vlc_queue) && vlc_mutex_lock==0){
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
//	int i,temp,len,sent_bytes;
//	struct sockaddr_ll dest_addr = {0};
//	struct ether_header *send_eh;
//	uint8_t send_eth_buff[BUF_SIZ];
//
//	//Setup Destination Address with
//	dest_addr.sll_ifindex = IF_IDX_GATEWAY_ETH;
//	dest_addr.sll_halen = ETHER_ADDR_LEN;
//	dest_addr.sll_family = AF_PACKET;
//	dest_addr.sll_protocol = htons(ETH_P_ALL);
//	for(i=0;i<6;i++){
//		dest_addr.sll_addr[i] = gateway_address[i];
//	}
//
//	for(;;){
//		len=0;
//
//		//Frame construction
//		send_eh = (struct ether_header*) send_eth_buff;
//		memset(send_eth_buff,0,BUF_SIZ);
//		for(i=0;i<6;i++){
//			send_eh->ether_shost[i] = ((uint8_t *)&eth_if_mac.ifr_hwaddr.sa_data)[i];
//		}
//		for(i=0;i<6;i++){
//			send_eh->ether_dhost[i] = gateway_address[i];
//		}
//		send_eh->ether_type = htons(ETH_P_ALL);
//		len+=sizeof(struct ether_header);
//
//		send_eth_buff[len++] = (unsigned char) (SELF_ID & 0xff);
//		memcpy(&send_eth_buff[len],(unsigned char*)&queue_size,sizeof(int));
////		printf("queue size of %d is reported\n",queue_size);
//		len+=sizeof(int);
//
//		do{
//			sent_bytes = sendto(eth_fd,send_eth_buff,len,0,(struct sockaddr*)&dest_addr,sizeof(struct sockaddr_ll));
//			if(sent_bytes==-1){
//				printf("%s\n",strerror(errno));
//			}
//		}
//		while(sent_bytes==-1);
//		printf("%d bytes of report are sent\n",sent_bytes);
//		usleep(REPORT_PERIOD);
//	}
//
//	pthread_exit(NULL);
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

	struct sockaddr *their_addr;
	socklen_t addr_len = sizeof their_addr;
	uint8_t rcv_eth_buff[BUF_SIZ];
	unsigned char* packet_content;
	struct ether_header *rcv_eh;
	int i;

	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
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
////		Read from Ethernet Socket. If exist, add to queue
//		rcv_eth_numbytes = recvfrom(eth_fd,rcv_eth_buff,BUF_SIZE,0,their_addr, &addr_len);
//		if(rcv_eth_numbytes > 0){
////			printf("%d bytes received from eth interface\n");
//			rcv_eh = (struct ether_header *) rcv_eth_buff;
////			if(address_match(rcv_eh)){
////				packet_content = (unsigned char *) (rcv_eth_buff + sizeof(struct ether_header));
//////				printf("Received packet:\n");
//////				for(i =0; i<14;i++){
//////					printf("0x%02x ",packet_content[i]);
//////				}
//////				printf("-----------\n");
////
////				NODE *temp = (NODE *) malloc(sizeof(NODE));
////				int i;
////				struct gateway_packet buffer_content = generate_gateway_packet_from_bytes(packet_content);
////
////				temp->data.node_id = buffer_content.node_id;
////				temp->data.vlc_id = buffer_content.vlc_id;
////				temp->data.payload_len = buffer_content.payload_len;
////				temp->data.seq_number = buffer_content.seq_number;
////				temp->data.data = (uint8_t *) malloc(buffer_content.payload_len * sizeof(uint8_t));
////				for(i=0;i<buffer_content.payload_len;i++){
////					temp->data.data[i] = buffer_content.data[i];
////				}
//////
////				if((queue_size+1) < QUEUE_LIMIT){	//IF the number of packets in queue already on the limit, the packets will be dropped
////					vlc_mutex_lock = 1;
////					Enqueue(vlc_queue,temp);
////					vlc_mutex_lock = 0;
////					printf("Added a packet for NODE-%d\n",buffer_content.node_id);
////					queue_size++;
////				}
////
//////				usleep(SCHEDULING_PERIOD);
////			}
//		}
//	}

	pthread_exit(NULL);
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

//	int if_name_len;
//	char *if_name = INTERFACE;
//	struct sockaddr_ll eth_addr;
//
//	//Domain Packet Socket, Socket type Raw (define header manually), Protocol for experimental
//	eth_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
//
//	if(eth_fd == -1){
//		printf("%s\n", strerror(errno));
//		return -1;
//	}
//
//	//Determine the Index number of interface used
//	if_name_len = strlen(if_name);
//	if(if_name_len < sizeof(ifr.ifr_name)){
//		memcpy(ifr.ifr_name, if_name, if_name_len);
//		ifr.ifr_name[if_name_len] = 0;
//
//		memcpy(if_mac.ifr_name, if_name, if_name_len);
//		if_mac.ifr_name[if_name_len] = 0;
//	}
//	else{
//		printf("interface name too long\n");
//		return -1;
//	}
//
//	if(ioctl(eth_fd,SIOCGIFINDEX,&ifr) == -1){
//		printf("%s", strerror(errno));
//		return -1;
//	}
//
//	printf("ETH IF Index: %d\n", ifr.ifr_ifindex);
//
//	if(ioctl(eth_fd,SIOCGIFHWADDR, &if_mac) <0){
//		printf("%s\n", strerror(errno));
//	}
//
//
//	//BIND ADDRESS FOR RECEIVING PACKETS FROM GATEWAY
//	eth_addr.sll_protocol = htons(ETH_P_ALL);
//	eth_addr.sll_family = PF_PACKET;
//	eth_addr.sll_ifindex = ifr.ifr_ifindex;
//	memcpy(eth_addr.sll_addr,(unsigned char *)if_mac.ifr_hwaddr.sa_data,ETHER_ADDR_LEN);
//
//	if(bind(eth_fd,(struct sockaddr *)&eth_addr,sizeof(eth_addr)) == -1){
//		printf("%s", strerror(errno));
//		return -1;
//	}
//
//	return 1;
}

//int address_match(struct ether_header *rcv_eh){
////	printf("0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",rcv_eh->ether_dhost[0],rcv_eh->ether_dhost[1],rcv_eh->ether_dhost[2],rcv_eh->ether_dhost[3],rcv_eh->ether_dhost[4],rcv_eh->ether_dhost[5]);
//	return rcv_eh->ether_dhost[0] == DEST_MAC0 &&
//			rcv_eh->ether_dhost[1] == DEST_MAC1 &&
//			rcv_eh->ether_dhost[2] == DEST_MAC2 &&
//			rcv_eh->ether_dhost[3] == DEST_MAC3 &&
//			rcv_eh->ether_dhost[4] == DEST_MAC4 &&
//			rcv_eh->ether_dhost[5] == DEST_MAC5 &&
//			rcv_eh->ether_shost[0] == gateway_address[0] &&
//			rcv_eh->ether_shost[1] == gateway_address[1] &&
//			rcv_eh->ether_shost[2] == gateway_address[2] &&
//			rcv_eh->ether_shost[3] == gateway_address[3] &&
//			rcv_eh->ether_shost[4] == gateway_address[4] &&
//			rcv_eh->ether_shost[5] == gateway_address[5]
//			;
//}

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

