#include "node.h"

struct node_status status;
char *report_bytes;
int report_length = 0;

int is_vlc_quality_flag = 0;
int is_reporting_flag = 0;

double ** VLC_LQ;
int *Frame_Rx;

//VLC SOCKET VARIABLES
int vlc_fd;
unsigned char buff[BUF_SIZE];
struct sockaddr_ll addr = {0};
socklen_t fromlen;
ssize_t vlc_bytes = 0;

#define MY_MAC0	0xec
#define MY_MAC1	0x08
#define MY_MAC2	0x6b
#define MY_MAC3	0x13
#define MY_MAC4	0x1e
#define MY_MAC5	0x5c

#define GATEWAY_MAC0 0x18
#define GATEWAY_MAC1 0xa6
#define GATEWAY_MAC2 0xf7
#define GATEWAY_MAC3 0x1a
#define GATEWAY_MAC4 0xa9
#define GATEWAY_MAC5 0x04

#define ETHER_TYPE	0x0800

#define RF_IF_NAME	"ra0"
#define BUF_SIZ		1024

#define N_VLC 1


int rf_fd;
struct ifreq rf_if_mac;	//Mac Address of the

packet_queue * process_queue;

//DISPLAYING RESULT
int from_vlc = 0;
int from_rf = 0;
struct timespec beginning, current_time;
int beginning_flag = 0;
int rs_error_bytes = 0;


int itval = 0;

static pthread_mutex_t process_queue_mutex;


unsigned char gateway_address[] = {
		0x18,0xa6,0xf7,0x1a,0xa9,0x04
};

void init_vlc_quality(){
	int i;

	VLC_LQ = (double **) malloc(N_VLC * sizeof(double *));
	for(i=0;i<N_VLC;i++){
		VLC_LQ[i] = (double *) malloc(1 * sizeof(double));
		VLC_LQ[i][0] = (double) 0;
	}
}

void *receive_vlc(void *tid){
	int i,idx;
	uint16_t packet_length;
	struct node_packet arrived_packet;
	NODE *pN;
	uint8_t temp_packet_content;
	unsigned char *int_temp = (unsigned char *) malloc(sizeof(int) * sizeof(unsigned char));
	uint32_t curr_seq_number = 0;
	int dropped_bytes = 0;
	int total_packet = 0;
	int curr_num_error = 0;
	int uncorrected_bytes = 0;
	int is_first_received = 1;
	int total_bytes = 0;

//	//For Bandwidth checking
//	struct timeval t1, t2;
//	double elapsedTime;

//	gettimeofday(&t1, NULL);
	for(;;){
		vlc_bytes = recvfrom(vlc_fd,buff,BUF_SIZE,0,(struct sockaddr*)&addr, &fromlen);
		if(vlc_bytes>0){
//			printf("Some bytes received: %d\n",vlc_bytes);
//			for(i=0;i<vlc_bytes;i++){
//				printf("0x%02x ",buff[i]);
//			}
//			printf("\n");

			packet_length = ((uint16_t)buff[1]<<8) + (uint16_t)buff[0];

			if(packet_length>0 && packet_length <= MTU_VALUE){
				arrived_packet = generate_from_bytes(buff);
				total_packet++;
				////[DEBUG]Checking the value of constructed packet
//				print_node_packet(arrived_packet);

				if(arrived_packet.node_id == SELF_ID || arrived_packet.node_id == DUMMY_PACKET_NODE_ID || arrived_packet.node_id == RS_ERROR_ID){
					//For all received packets, Set the Frame Rx value of respected VLC-Tx
					if(arrived_packet.vlc_id > 0 && arrived_packet.vlc_id<=N_VLC){
						Frame_Rx[arrived_packet.vlc_id-1] = 1;
//						printf("Frame RX is updated\n");
					}

					if(arrived_packet.node_id == SELF_ID || arrived_packet.node_id == RS_ERROR_ID){
//						print_node_packet(arrived_packet); //[DEBUG]Check value of received packets
//						printf("Packet with node id %d is added to process queue\n",arrived_packet.node_id);
//						printf("Packet with %d Seq Number is received\n",arrived_packet.seq_number);

						if(arrived_packet.node_id == RS_ERROR_ID){
							memcpy((uint8_t*)&curr_num_error,arrived_packet.data,sizeof(unsigned int));
//							printf("--------------\nCurrent num error: %d\n--------------\n",curr_num_error);
							if(curr_num_error < 0){
								uncorrected_bytes -= curr_num_error;
							}
							else{
								rs_error_bytes+= curr_num_error;
							}

							arrived_packet.node_id = SELF_ID; //To handle packets which node id is altered for
						}

						pN = (NODE *) malloc(sizeof(NODE));
						pN->data = arrived_packet;

//						printf("----------------\nPacket with Seq Number: %d\n----------------\n",arrived_packet.seq_number);
						pthread_mutex_lock(&process_queue_mutex);
						push(process_queue,pN);
						pthread_mutex_unlock(&process_queue_mutex);

						from_vlc++;

						if(is_first_received){
							curr_seq_number = arrived_packet.seq_number;
							is_first_received = 0;
						}
						else{
							if(curr_seq_number == (MAX_SEQ_NUMBER-1)){
								if(arrived_packet.seq_number != 0){
									dropped_bytes += (arrived_packet.seq_number * packet_length);
								}
							}
							else{
								if(arrived_packet.seq_number != (curr_seq_number+1)){
//									printf("Arrived seq: %d\nCurrent Seq: %d\n",arrived_packet.seq_number,curr_seq_number);
//									dropped_packet++;
//									dropped_bytes += (arrived_packet.seq_number - curr_seq_number) * packet_length;
									dropped_bytes += packet_length;
//									printf("%d dropped packets from % received\n",dropped_packet,total_packet);
								}
							}
						}

						curr_seq_number = arrived_packet.seq_number;
						total_bytes += packet_length;
					}

//					total_bytes += packet_length;
				}
			}
		}

//		gettimeofday(&t2, NULL);
//		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
//		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

//		if(elapsedTime > 60000){
//			printf("total bytes: %d\n",total_bytes);
////			printf("dropped bytes: %d\n",dropped_bytes);
//			printf("Uncorrected bytes: %d\n",uncorrected_bytes);
//			printf("rs_error_bytes bytes: %d\n",rs_error_bytes);
//			total_bytes = 0;
//			dropped_bytes = 0;
//			rs_error_bytes = 0;
//
//			gettimeofday(&t1, NULL);
//		}
	}
	pthread_exit(NULL);
}

int is_from_gateway(struct ether_header *eh){
//	printf("Sender Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);

	return (eh->ether_shost[0] == GATEWAY_MAC0 &&
 	 	 	 eh->ether_shost[1] == GATEWAY_MAC1 &&
			 eh->ether_shost[2] == GATEWAY_MAC2 &&
			 eh->ether_shost[3] == GATEWAY_MAC3 &&
			 eh->ether_shost[4] == GATEWAY_MAC4 &&
			 eh->ether_shost[5] == GATEWAY_MAC5 &&
			 eh->ether_dhost[0] == MY_MAC0 &&
			 eh->ether_dhost[1] == MY_MAC1 &&
			 eh->ether_dhost[2] == MY_MAC2 &&
			 eh->ether_dhost[3] == MY_MAC3 &&
			 eh->ether_dhost[4] == MY_MAC4 &&
			 eh->ether_dhost[5] == MY_MAC5);
}

void *receive_rf(void *tid){
	int i;
	struct node_packet arrived_packet;
	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
	NODE *pN;
	struct ether_header *eh = (struct ether_header *) buf;

	for(;;){
		numbytes = recvfrom(rf_fd, buf, BUF_SIZ, 0, NULL, NULL);
//		printf("listener: got packet %lu bytes\n", numbytes);

		/* Check the packet is for me */
		if (is_from_gateway(eh)) {
//				printf("received %d numbytes from RF\n",numbytes);

			report_content = (unsigned char*) (buf + sizeof(struct ether_header));
			int idx = 0;
			memcpy((unsigned char*)&arrived_packet.node_id,&report_content[idx],sizeof(uint16_t));
			idx+=sizeof(uint16_t);
			memcpy((unsigned char*)&arrived_packet.vlc_id,&report_content[idx],sizeof(uint16_t));
			idx+=sizeof(uint16_t);
			memcpy((unsigned char*)&arrived_packet.payload_len,&report_content[idx],sizeof(uint16_t));
			idx+=sizeof(uint16_t);
			memcpy((unsigned char*)&arrived_packet.seq_number,&report_content[idx],sizeof(uint32_t));
//			printf("----------------\nPacket with Seq Number: %d\n----------------\n",arrived_packet.seq_number);
			idx+=sizeof(uint32_t);
			arrived_packet.data = (unsigned char *) malloc(arrived_packet.payload_len * sizeof(unsigned char));
			for(i=0;i<arrived_packet.payload_len;i++){
				arrived_packet.data[i] = report_content[idx++];
			}

			pN = (NODE *) malloc(sizeof(NODE));
			pN->data = arrived_packet;

			pthread_mutex_lock(&process_queue_mutex);
			push(process_queue,pN);
			pthread_mutex_unlock(&process_queue_mutex);

			from_rf++;
		}
	}

	pthread_exit(NULL);
}

void *process_data(void *tid){
	int i;
	struct node_packet processed_data;
	NODE *temp;

	for(;;){
		if(!is_empty(process_queue)){

			pthread_mutex_lock(&process_queue_mutex);
			temp = pop(process_queue);
			pthread_mutex_unlock(&process_queue_mutex);

			if(temp){
				processed_data = temp->data;
	//			free(temp);
				i = (int) processed_data.seq_number;
	//			printf("Data with seq number %d is processed\n",processed_data.seq_number);

	//			free(processed_data.data);

				if(i>=status.rcv_seq){
					status.rcv_state[i] = 1;
				}


				if(i>status.last_rcv_seq){
					status.last_rcv_seq = i;
				}

				while(status.rcv_state[status.rcv_seq] == 1){
					status.rcv_seq+=1;
				}

				if(status.last_rcv_seq - status.rcv_seq + 1 > max_ACK_bit_pattern){
					status.rcv_seq = status.last_rcv_seq - max_ACK_bit_pattern + 1;
				}
			}

		}
	}

	pthread_exit(NULL);
}

void *vlc_quality_measurement(void *tid){
	int i;

	for(;;){
		for(i=0;i<N_VLC;i++){
//			printf("when read Frame Rx is %d\n",Frame_Rx[i]);
			VLC_LQ[i][0] = ((double)1 - ALPHA) * VLC_LQ[i][0] + ALPHA * (double) Frame_Rx[i];
//			printf("VLC LQ at measurement: %f \n",VLC_LQ[i][0]);
			memset(Frame_Rx,0,N_VLC * sizeof(int)); //Reset the Frame_Rx for the next period
		}
		usleep(VLC_Q_PERIOD);
	}
	pthread_exit(NULL);
}

void *output_data(void *tid){
	int i;
	double x_axis;
	for(;;){
		if(from_vlc > 0 || from_rf >0){
			if(beginning_flag == 0){
				clock_gettime(CLOCK_MONOTONIC,&beginning);
				beginning_flag = 1;
			}

			clock_gettime(CLOCK_MONOTONIC,&current_time);
			x_axis = ((double)(current_time.tv_sec - beginning.tv_sec) * 1000000000L  + (double)(current_time.tv_nsec - beginning.tv_nsec))/1000;

			printf("%f,%d,%d\n",x_axis,from_vlc,from_rf);
			usleep(OUTPUT_PERIOD);
		}
	}

	pthread_exit(NULL);
}

void *send_report(void *tid){

	//Variables for Report Transmission
	int rf_bytes_sent;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	int i,j, pattern_bytes;
	int unsigned_char_size,remaining_pattern_length,cur_idx,cur_last_idx,n_shift;
	uint8_t node_id = SELF_ID;
	int pattern_length;

	socket_address.sll_ifindex = RF_IF_IDX;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = GATEWAY_MAC0;
	socket_address.sll_addr[1] = GATEWAY_MAC1;
	socket_address.sll_addr[2] = GATEWAY_MAC2;
	socket_address.sll_addr[3] = GATEWAY_MAC3;
	socket_address.sll_addr[4] = GATEWAY_MAC4;
	socket_address.sll_addr[5] = GATEWAY_MAC5;

	for(;;){
		memset(sendbuf, 0, BUF_SIZ);
		/* Ethernet header */
		eh->ether_shost[0] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[0];
		eh->ether_shost[1] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[1];
		eh->ether_shost[2] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[2];
		eh->ether_shost[3] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[3];
		eh->ether_shost[4] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[4];
		eh->ether_shost[5] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[5];
	//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
		eh->ether_dhost[0] = GATEWAY_MAC0;
		eh->ether_dhost[1] = GATEWAY_MAC1;
		eh->ether_dhost[2] = GATEWAY_MAC2;
		eh->ether_dhost[3] = GATEWAY_MAC3;
		eh->ether_dhost[4] = GATEWAY_MAC4;
		eh->ether_dhost[5] = GATEWAY_MAC5;
	//	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
		/* Ethertype field */
		eh->ether_type = htons(ETH_P_ALL);
		tx_len = sizeof(struct ether_header);

		/* Packet data */
		sendbuf[tx_len++] = (unsigned char) (node_id & 0xff);
	//	printf("node id: %d\n",packet.node_id);
		memcpy(&sendbuf[tx_len],(unsigned char *) &status.rcv_seq,sizeof(int));
	//	printf("ACK number: %d\n",status.rcv_seq);
		tx_len+=sizeof(int);

		pattern_length = status.last_rcv_seq-status.rcv_seq+1;
//			printf("Last rcv seq:  %d rcv_seq: %d\n",status.last_rcv_seq,status.rcv_seq);
		memcpy(&sendbuf[tx_len],(unsigned char *) &pattern_length,sizeof(int));
//			printf("Pattern length: %d\n",pattern_length);
		tx_len+=sizeof(int);

		unsigned_char_size = sizeof(unsigned char) * 8;
		pattern_bytes = pattern_length / unsigned_char_size + (pattern_length % unsigned_char_size == 0 ? 0 : 1);

		remaining_pattern_length = pattern_length;

		for(i=0;i<pattern_bytes;i++){
			unsigned char temp_byte = (unsigned char)(0 & 0x0);

			cur_idx = i * unsigned_char_size;
			if(remaining_pattern_length > unsigned_char_size){
				cur_last_idx = cur_idx + unsigned_char_size;
			}
			else{
				cur_last_idx =  cur_idx + remaining_pattern_length;
			}

			n_shift = unsigned_char_size - 1;
			for(j=cur_idx;j<cur_last_idx;j++){
				temp_byte = temp_byte | (status.rcv_state[status.rcv_seq+j]& 0x1)<<n_shift; //Convert each character (0 or 1) in the packet bit pattern to a bit
//					temp_byte = temp_byte | (packet.ACK_bit_pattern[j]& 0x1)<<n_shift; //Convert each character (0 or 1) in the packet bit pattern to a bit
				n_shift--;
			}
			//printf("pattern byte %d: %08x\n",i,temp_byte);
			sendbuf[tx_len++] = temp_byte;
			remaining_pattern_length -= unsigned_char_size;
		}

		sendbuf[tx_len++] = (unsigned char) (N_VLC & 0xff);

		for(i=0;i<N_VLC;i++){
			sendbuf[tx_len++] = (unsigned char) ((i+1) & 0xff); //VLC TX ID
			int in_255_range = (int)(VLC_LQ[i][0] * (double) 255);
//				printf("VLC Link Quality: %d\n",in_255_range);
			sendbuf[tx_len++] = (unsigned char) (in_255_range & 0xff);
		}

		do{
			rf_bytes_sent = sendto(rf_fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
			if (rf_bytes_sent < 0){
				printf("Send failed\n");
			}
		} while(rf_bytes_sent < 0);

		printf("%d report bytes sent\n",rf_bytes_sent);
		usleep(REPORT_PERIOD);
	}

	pthread_exit(NULL);
}

int main(int argc, char* argv[]){

	int i, bit_pattern_idx;

	struct status_report_packet report;

	//INITIALIZATION
	status = initialize_node_status(MAX_SEQ_NUMBER);
	init_vlc_quality();
	Frame_Rx = (int *) malloc(N_VLC * sizeof(int));
	memset(Frame_Rx,0,N_VLC * sizeof(int));

	setup_vlc_socket();
	setup_rf_socket();
	process_queue = construct_queue(MAX_PROCESS_QUEUE);
	//INIT THREAD
	pthread_t threads[N_THREAD];

	if(pthread_create(&threads[RCV_VLC_TID],NULL,receive_vlc,(void *)RCV_VLC_TID)
			|| pthread_create(&threads[RCV_RF_TID],NULL,receive_rf,(void *)RCV_RF_TID)
			|| pthread_create(&threads[PROC_DATA_TID],NULL,process_data,(void *)PROC_DATA_TID)
			|| pthread_create(&threads[VLC_Q_TID],NULL,vlc_quality_measurement,(void *)VLC_Q_TID)
//			|| pthread_create(&threads[OUTPUT_TID],NULL,output_data,(void *)OUTPUT_TID)
			|| pthread_create(&threads[SEND_REPORT_TID],NULL,send_report,(void *)SEND_REPORT_TID)
			){
		printf("error initializing thread\n");
		exit(-1);
	}

	for(i=0;i<N_THREAD;i++){
		pthread_join(threads[i],NULL);
	}

	return 0;
}

int setup_vlc_socket(){
	const unsigned char ether_broadcast_addr[] = {
			0xff,0xff,0xff,0xff,0xff,0xff
	};
	char sender[INET6_ADDRSTRLEN];
	int ret, i,if_name_len,if_idx;
	struct packet_mreq mr;
	int sockopt;
	struct ifreq ifr;
	char *if_name = VLC_INTERFACE_NAME;
	struct ether_header *eh;
	int is_repeat = 1;

	//Domain Packet Socket, Socket type Raw (define header manually), Protocol for experimental
	vlc_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	if(vlc_fd == -1){
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

	if(ioctl(vlc_fd,SIOCGIFINDEX,&ifr) == -1){
		printf("fail to get index of interface");
	}
	printf("VLC IF Index: %d\n", ifr.ifr_ifindex);

	//Construct Destination Address
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifr.ifr_ifindex;
	addr.sll_halen = ETHER_ADDR_LEN;
	addr.sll_protocol = htons(ETH_P_ALL);
	memcpy(addr.sll_addr,ether_broadcast_addr,ETHER_ADDR_LEN);

	if(bind(vlc_fd,(struct sockaddr*)&addr, sizeof(addr))<0){
		printf("error binding\n");
	}
	fromlen = sizeof(struct sockaddr_ll);

//	//Set the option of the socket
//	memset(&mr,0,sizeof(mr));
//	mr.mr_ifindex = ifr.ifr_ifindex;
//	mr.mr_type = PACKET_MR_PROMISC; //TODO: Dont know what type is the correct one
//	setsockopt(vlc_fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr));
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
