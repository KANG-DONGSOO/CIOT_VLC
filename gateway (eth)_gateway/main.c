#include "gateway.h"

// static 변수 = 정적변수, 프로그램 실행 시 할당되고, 프로그램 종료시 파괴되는 변수
// 선언시 초기화 하지 않아도 0으로 초기화된다.
// 함수가 여러번 실행되더라도 변수의 선언 및 초기화는 한 번만 이루어진다.!
static struct gateway_status status;
// packet_queue 자료형 = Queue[NODE *head, NODE *tail, size, limit]
static packet_queue **node_queues;
static packet_queue *rf_queue;
static packet_queue *vlc_queue;

int is_scheduling_flag = 1;

//RF Socket Variables
int sockfd,numbytes;
struct ifreq rf_if_idx,rf_if_mac;

//VLC Socket variables
int eth_fd;
struct ifreq eth_if_mac;	//Mac Address of the Ethernet Interface

//Algorithm 3 variables
int *gn; //Queue length for node n at epoch t in gateway
int *sn; //Scheduled method at epoch t for node n
int *an; //Number of packets arrived for node n at epoch t
int d_rf; //Number of departure packets from RF in an epoch
int *d_vlc; //Number of departure packets for VLC TX k in an epoch
int r; //Length of RF Queue
double** e; //Error Probability from VLC TX k to node n
int **E; //Indicator if a packet from VLC TX K already received by node N
int *vk; //Length of queue for VLC TX K

//TBF VARIABLES
int n_token = 0;
int tbf_queue_length = 0;
packet_queue *tbf_queue;

//Unacked VLC Variables
int n_unacked_vlc_packet;
static struct gateway_packet unacked_vlc_packet[MAX_UNACKED_VLC_PACKET];
// MAX_UNACKED_VLC_PACKET = 500000

//struct node_address *node_addresses;
unsigned char ** vlc_addresses;
unsigned char ** node_addresses;

//For Thread Synchronizing
static pthread_mutex_t node_queue_mutex;
static pthread_mutex_t rf_queue_mutex;
static pthread_mutex_t tbf_queue_mutex;
static pthread_mutex_t vlc_queue_mutex;

static unsigned char rf_if_mac_address[] = {
		0x18,0xa6,0xf7,0x1a,0xa9,0x04
};

static unsigned char eth_if_mac_address[] = {
		0xa0,0xf6,0xfd,0x64,0xaf,0xfc
};

//int after_n_report=0;

void handle_scheduler(int sig){
	is_scheduling_flag = 1;
}

// node 주소 설정.
void setup_node_address(){
	// N_NODE 크기의 바이트만큼 node_addresses의 2차 포인터에 메모리 동적할당.
	node_addresses = (unsigned char**) malloc(N_NODE * sizeof(unsigned char*));


	//Fill manually the addresses of nodes; 직접 node들의 주소를 채움.
	// 1차배열 node_addresses에 6바이트만큼 메모리를 동적할당함.
	// 2차 배열에 주소를 할당.
	node_addresses[0] = (unsigned char *) malloc(6 * sizeof(unsigned char));
	node_addresses[0][0] = 0xec;
	node_addresses[0][1] = 0x08;
	node_addresses[0][2] = 0x6b;
	node_addresses[0][3] = 0x13;
	node_addresses[0][4] = 0x1e;
	node_addresses[0][5] = 0x5c;

	node_addresses[1] = (unsigned char *) malloc(6 * sizeof(unsigned char));
	node_addresses[1][0] = 0xec;
	node_addresses[1][1] = 0x08;
	node_addresses[1][2] = 0x6b;
	node_addresses[1][3] = 0x13;
	node_addresses[1][4] = 0x1e;
	node_addresses[1][5] = 0x5c;
}

// VLC 주소 설정.
void setup_vlc_address(){
	// N_VLC x 1바이트 크기만큼 메모리를 동적 할당함, 1바이트크기의 2차 구조체로 캐스팅
	// uint8_t = unsigned char = 1바이트
	vlc_addresses = (uint8_t **) malloc(N_VLC * sizeof(uint8_t*));

	//TODO: FILL THE MAC ADDRESS OF VLC TXs
	// 1차 배열 vlc_addresses에 6바이트만큼 메모리 동적할당.
	// 2차 배열로 주소를 직접 입력
	vlc_addresses[0] = (uint8_t *) malloc(6 * sizeof(uint8_t));
	vlc_addresses[0][0] = 0xb0;
	vlc_addresses[0][1] = 0xd5;
	vlc_addresses[0][2] = 0xcc;
	vlc_addresses[0][3] = 0xfc;
	vlc_addresses[0][4] = 0xfe;
	vlc_addresses[0][5] = 0x85;

	vlc_addresses[1] = (uint8_t *) malloc(6 * sizeof(uint8_t));
	vlc_addresses[1][0] = 0xb0;
	vlc_addresses[1][1] = 0xd5;
	vlc_addresses[1][2] = 0xcc;
	vlc_addresses[1][3] = 0xff;
	vlc_addresses[1][4] = 0x7b;
	vlc_addresses[1][5] = 0xef;
}

// packet_queue.h 에 typedef struct Node_t -> NODE;
NODE *generate_random_node(int node_id){
	NODE *node_temp = (NODE*) malloc(sizeof(NODE));

	struct gateway_packet temp;
	int i;

	temp.payload_len = PAYLOAD_SIZE;
	temp.node_id = node_id;
	temp.seq_number = 0;
	temp.vlc_id = 1;
	temp.data = (unsigned char*) malloc(temp.payload_len *sizeof(unsigned char));
	for(i=0;i<temp.payload_len;i++){
		temp.data[i] = (unsigned char)(rand() & 0x00ff);
	}

	node_temp->data = temp;
	// (*node_temp).data = temp;와 같은 말이다.
	// node_temp에 저장된 주소에 가서 data요소에 값 temp를 대입함.
	return node_temp;
}

// algorithm_3_variables의 초기 설정
void initialize_algorithm_3_variables(){
	int i,j;

	r = 0;
	d_rf = 0;
	d_vlc = (int *) malloc(N_VLC * sizeof(int));
	// memset(배열의시작주소, 초기화할 값, 배열의크기); 배열을 초기화 시켜준다.
	memset(d_vlc,0,N_VLC * sizeof(int));

	gn = (int *) malloc(N_NODE * sizeof(int)); // Queue Length for node n at epoch t in gateway
	sn = (int *) malloc(N_NODE * sizeof(int)); // scheduled method at epoch t for node n
	an = (int *) malloc(N_NODE * sizeof(int)); // number of packets arrived for node n at epoch t

	// memset 함수로 gn, sn, an의 배열을 초기화한다.
	memset(gn,0, N_NODE * sizeof(int));
	memset(sn,0, N_NODE * sizeof(int));
	memset(an,0, N_NODE * sizeof(int));

	// VLC tx k 에서 node n 으로의 에러 확률
	e = (double **) malloc(N_VLC * sizeof(double *));
	for(i=0;i<N_VLC;i++)
		e[i] = (double *) malloc(N_NODE * sizeof(double));

	for(i=0;i<N_VLC;i++)
		for(j=0;j<N_NODE;j++)
			e[i][j] = 0;

	vk = (int *) malloc(N_VLC * sizeof(int));
	memset(vk,0,N_VLC*sizeof(int));

	//Indicator if a packet from VLC TX K already received by node N
	// VLC TX K의 node N으로 packet을 이미 수신받았을 때의 지표.
	E = (int **) malloc(N_VLC * sizeof(int *));
	for(i=0;i<N_VLC;i++)
		E[i] = (int *) malloc(N_NODE * sizeof(int));

	for(i=0;i<N_VLC;i++)
		for(j=0;j<N_NODE;j++)
			E[i][j] = 0;
}


int calculate_v(int vlc_id){
	int i,temp = 0;

	temp = (vk[vlc_id]-d_vlc[vlc_id] > 0 ? vk[vlc_id]-d_vlc[vlc_id] : 0);
	for(i=0;i<N_NODE;i++){
		temp+= is_k_in_sn(vlc_id+1,sn[i]);
	}

	return temp;
}

void print_variables_of_scheduling(int node_id){
	int i;

	printf("Queue Length for Node %d: %d\n",node_id+1,gn[node_id]);
	printf("Scheduled Method for Node %d at previous epoch: %d\n",node_id+1,sn[node_id]);
	printf("Number of packets arrived for Node %d at epoch: %d\n",node_id+1,an[node_id]);
	printf("Number of departing packet from RF in previous epoch: %d\n",d_rf);
	printf("Number of departing packet from VLC TX in an epoch:\n");
	for(i=0;i<N_VLC;i++){
		printf("VLC-%d: %d\n",i+1,d_vlc[i]);
	}
	printf("Length of RF Queue: %d\n",r);
	printf("Error Probability for VLC TX:\n");
	for(i=0;i<N_VLC;i++){
		printf("VLC-%d: %f\n",i+1,e[i][node_id]);
	}
	printf("Received Packets indicator for VLC TX:\n");
	for(i=0;i<N_VLC;i++){
		printf("VLC-%d: %d\n",i+1,E[i][node_id]);
	}
	printf("Length of Queue for VLC TX:\n");
	for(i=0;i<N_VLC;i++){
		printf("VLC-%d: %d\n",i+1,vk[i]);
	}
	printf("\n");
}

struct scheduling_result schedule(int node_id){
	struct scheduling_result retval;
	int mode_temp = IDLE_MODE,i,j,v_sum = 0,n_vlc_src = N_VLC, n_vlc_tgt=0,vlc_tmp;
	double err = 1,tgt = 0;

	//printf("\n\nSCHEDULING FOR NODE %d\n",node_id+1);
	//print_variables_of_scheduling(node_id);

	if((gn[node_id] * -1) + r < 0){
		mode_temp = RF_MODE;
		tgt = (gn[node_id] * -1) + r;
	}

	int *vlc_src_set = (int *) malloc(N_VLC * sizeof(int));
	int *vlc_tgt_set = (int *) malloc(N_VLC * sizeof(int));
	for(i=0;i<N_VLC;i++){
		vlc_src_set[i] = i; //Initialize VLC SRC SET with all existing VLC TXs
	}

	while(n_vlc_src > 0){
		vlc_tmp = -1; //Changed to -1 because VLC ID started at 0
		for(i=0;i<n_vlc_src;i++){
//			vk[i] = calculate_v(i);
			double temp = (gn[node_id]*-1)+v_sum+vk[i]+(r*err*e[i][node_id]);
			if(temp<tgt){
				vlc_tmp = i;
				tgt = temp;
			}
		}
		if(vlc_tmp != -1){
			mode_temp = VLC_MODE;
			v_sum+=vk[vlc_tmp];
			err*=e[vlc_tmp][node_id];

			int vlc_tmp_idx = -1;
			for(i=0;i<n_vlc_src;i++){
				if(vlc_src_set[i]==vlc_tmp){
					vlc_tmp_idx=i;
					break;
				}
			}
			if(vlc_tmp_idx>=0){
				vlc_src_set[vlc_tmp_idx] = vlc_src_set[n_vlc_src-1];
				n_vlc_src-=1; //REMOVE VLC TMP FROM VLC SRC SET
			}


			vlc_tgt_set[n_vlc_tgt] = vlc_tmp;
			n_vlc_tgt+=1; //ADD VLC TMP TO VLC TGT SET
		}
		else{
			break;
		}
	}

//	retval.mode = mode_temp;
//	retval.vlc_tgt_set = vlc_tgt_set;
//	retval.n_vlc_tgt = n_vlc_tgt;

	free(vlc_src_set);
	//[TESTING] - SCHEDULING PACKET TO VLC OR RF
	retval.mode = 0; //Set 0 for RF, 1 for VLC (1 TX only)
	vlc_tgt_set[0] = 0;
	retval.vlc_tgt_set = vlc_tgt_set;
	retval.n_vlc_tgt = 1;


//	if(retval.mode>RF_MODE){
//		printf("----------------------------------\nScheduled to VLC\n----------------------------------\n");
//	}
//	printf("\n\nRESULT:\n");
//	if(retval.mode ==IDLE_MODE)
//		printf("IDLE\n");
//	else if(retval.mode ==RF_MODE)
//		printf("RF\n");
//	else
//		printf("VLC\n");
//
//	printf("\n");

	return retval;
}

// transmit_rf 함수를 실행,
void *transmit_rf(void *thread_id){
	//TOKEN BUCKET FILTER IS IMPLEMENTED FOR RF TRANSMISSION
	for(;;){
		if(!is_empty(rf_queue) && tbf_queue_length < LIMIT){
//			printf("Added when tbf queue length = %d\n",tbf_queue_length);
			//SEND THE WAITING PACKETS FOR RF
			pthread_mutex_lock(&rf_queue_mutex);
			NODE *temp = pop(rf_queue);
			pthread_mutex_unlock(&rf_queue_mutex);

			if(temp != NULL){
				pthread_mutex_lock(&rf_queue_mutex);
				push(tbf_queue,temp);
				pthread_mutex_unlock(&rf_queue_mutex);

//				printf("Packet seq number %d added to TBF Queue\n",temp->data.seq_number);
				tbf_queue_length++;
				r--;
			}
		}
	}

	pthread_exit(NULL);
}

int is_from_node(struct ether_header *eh){
//	printf("Sender Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	int i;
	int is_node = 0;

	for(i=0;i<N_NODE;i++){
		is_node = (eh->ether_shost[0] == node_addresses[i][0] &&
				 eh->ether_shost[1] == node_addresses[i][1] &&
				 eh->ether_shost[2] == node_addresses[i][2] &&
				 eh->ether_shost[3] == node_addresses[i][3] &&
				 eh->ether_shost[4] == node_addresses[i][4] &&
				 eh->ether_shost[5] == node_addresses[i][5] &&
				 eh->ether_dhost[0] == RF_MAC0 &&
				 eh->ether_dhost[1] == RF_MAC1 &&
				 eh->ether_dhost[2] == RF_MAC2 &&
				 eh->ether_dhost[3] == RF_MAC3 &&
				 eh->ether_dhost[4] == RF_MAC4 &&
				 eh->ether_dhost[5] == RF_MAC5);
		if(is_node){
			break;
		}
	}

	return is_node;
}

void *receive_report(void *thread_id){
	int i,l;

	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
	struct ether_header *eh = (struct ether_header *) buf;
	int reporting_node_idx;

	for(;;){
		numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);

		if(numbytes > 0){
			if (is_from_node(eh)) {

				report_content = (unsigned char*) (buf + sizeof(struct ether_header));
				struct status_report_packet report = generate_status_report_from_string(report_content);

	//			//[TESTING]Checking the correctness of report content
				print_report_content(report);

				if(report.node_id > 0 && report.node_id <= N_NODE){
					reporting_node_idx = report.node_id - 1; //Because Idx starts from 0 and ID from 1
					status.unacked_seq[reporting_node_idx] = report.ACK_number;
					for(i=0;i<report.pattern_length;i++){
						int curr_unacked_packet = status.unacked_seq[reporting_node_idx] + i -1;
						if(curr_unacked_packet < MAX_SEQ_NUMBER){
							if(status.pkt_state[reporting_node_idx][curr_unacked_packet] == VLC_unacked &&
									report.ACK_bit_pattern[i] == '1'){
								status.pkt_state[reporting_node_idx][curr_unacked_packet] = VLC_acked;
								for(l=0;l<n_unacked_vlc_packet;l++){
									if(unacked_vlc_packet[l].node_id-1 == reporting_node_idx && unacked_vlc_packet[l].seq_number == curr_unacked_packet){
										unacked_vlc_packet[l] = unacked_vlc_packet[n_unacked_vlc_packet-1];
										n_unacked_vlc_packet--;
										break;
									}
								}
							}
						}
					}

					for(i=0;i<report.n_vlc;i++){
						struct vlc_information temp = report.vlc_data[i];
						int vlc_tx_idx = temp.vlc_tx_id-1;


						e[vlc_tx_idx][reporting_node_idx] = 1 - ((double)temp.vlc_link_quality/255);

//						//[TESTING]Value of VLC Tx Error for each Node
//						printf("e for vlc%d: %f\n",temp.vlc_tx_id,e[vlc_tx_idx][reporting_node_idx]);
					}

					free(report.ACK_bit_pattern);
					free(report.vlc_data);
				}

			}
		}
		/* Check the packet is for me */
	}

	pthread_exit(NULL);
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


void *receive_vlc_report(void *thread_id){
	int i;
	ssize_t numbytes;
	uint8_t buf[BUF_SIZ];
	unsigned char *report_content;
	struct ether_header *eh = (struct ether_header *) buf;
	int vlc_id,temp_queue,idx;

	for(;;){
		numbytes = recvfrom(eth_fd, buf, BUF_SIZ, 0, NULL, NULL);
//		printf("listener: got packet %lu bytes\n", numbytes);

		/* Check the packet is for me */
		if (is_vlc_tx_address(eh)) {
			idx=0;
			temp_queue=0;

			report_content = (unsigned char*) (buf + sizeof(struct ether_header));
			vlc_id = (int)report_content[idx++];

			if(vlc_id <= N_VLC && vlc_id > 0){	//CHECK IF THE RECEIVED VLC_ID IS VALID
				memcpy((unsigned char*)&temp_queue,&report_content[idx],sizeof(int));
				idx+=sizeof(int);
//				printf("received %d queue length for VLC-%d and %d bytes\n",temp_queue,vlc_id,numbytes);
				vk[vlc_id-1] = temp_queue;
			}
		}
	}

	pthread_exit(NULL);
}

void *transmit_vlc(void *tid){
	int sent_vlc_bytes;
	struct gateway_packet vlc_packet;
	//For Bandwidth checking
	struct timeval t1, t2;
	double elapsedTime;
	int total_bytes = 0;
	gettimeofday(&t1, NULL);

	for(;;){
		if(!is_empty(vlc_queue)){
			//SEND THE WAITING PACKETS FOR RF
			NODE *temp;
			pthread_mutex_lock(&vlc_queue_mutex);
			temp = pop(vlc_queue);
			pthread_mutex_unlock(&vlc_queue_mutex);

			if(temp != NULL){
				vlc_packet = temp->data;

				free(temp);

				do{
					sent_vlc_bytes = transmit_packet_vlc(vlc_packet);
//					printf("%d bytes sent to VLC\n",sent_vlc_bytes);
					usleep(1);
				} while(sent_vlc_bytes <=0);

				free(vlc_packet.data);
				d_vlc[vlc_packet.vlc_id]--;
				total_bytes += vlc_packet.payload_len; //[DEBUG]Checking the Data Input Rate
			}
		}

		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

		if(elapsedTime > 10000){
//			printf("total bytes: %d\n",total_bytes);
			total_bytes = 0;
			gettimeofday(&t1, NULL);
		}
	}

	pthread_exit(NULL);
}

void *add_token(void *tid){
	for(;;){
		int burst = BURST;
		int token_rate = TOKEN_RATE;

		if(burst - n_token >= token_rate){
			n_token+=TOKEN_RATE;
		}

		usleep(TOKEN_PERIOD);
	}

	pthread_exit(NULL);
}

void *tbf_transmit(void *tid){
	for(;;){
		if(!is_empty(tbf_queue) && n_token > 0){
			//TRANSMIT THE PACKET FROM TBF QUEUE
			NODE *temp;
			pthread_mutex_lock(&rf_queue_mutex);
			temp = pop(tbf_queue);
			pthread_mutex_unlock(&rf_queue_mutex);

			if(temp != NULL){
				if(transmit_packet_rf(temp->data) > 0){
					n_token--;
					tbf_queue_length--;

					free(temp);
				}
			}
		}
	}

	pthread_exit(NULL);
}

void *generate_packet(void *tid){
	NODE *pN;
	int i,j;

	//For Bandwidth checking
//	struct timeval t1, t2;
//	double elapsedTime;
//	int total_bytes = 0;
//	gettimeofday(&t1, NULL);
	for(;;){
		for (i = 0; i < N_NODE; i++) {
			int n_generated_packets = GENERATED_PACKET_PER_N_MICROSECOND;
			//GENERATE RANDOM NUMBER OF PACKETS FOR EACH NODE
			if(gn[i] + n_generated_packets < NODE_QUEUE_SIZE){
				for(j=0;j<n_generated_packets;j++){
					pN = generate_random_node(i+1);
					if(pN != NULL){
						pN->data.seq_number = status.arr_seq[i];
						status.arr_seq[i] = (status.arr_seq[i]+ 1) % MAX_SEQ_NUMBER;

	//					total_bytes += pN->data.payload_len; //[DEBUG]Checking the Data Input Rate

						pthread_mutex_lock(&node_queue_mutex);
						push(node_queues[i], pN);
						pthread_mutex_unlock(&node_queue_mutex);

						gn[i]++;
					}
				}
			}
		}

//		gettimeofday(&t2, NULL);
//		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
//		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
//
//		if(elapsedTime > 10000){
//			printf("total bytes: %d\n",total_bytes);
//			total_bytes = 0;
//			gettimeofday(&t1, NULL);
//		}

		usleep(N_MICROSECOND_PACKET_GENERATION);
	}

	pthread_exit(NULL);
}

void *check_unacked(void *tid){
	struct timespec current_time;
	double diff;
	int i,j,l;

	for(;;){

		for(i=0;i<N_NODE;i++){
		//printf("From: %d To: %d\n",status.unacked_seq[i],status.unsched_seq[i]);
			for(j=status.unacked_seq[i];j<status.unsched_seq[i];j++){

				clock_gettime(CLOCK_MONOTONIC,&current_time);
				diff = (double)(status.expire_time[i][j].tv_sec - current_time.tv_sec) * 1000000000L  + (double)(status.expire_time[i][j].tv_nsec - current_time.tv_nsec);
				//printf("DIFF RESULT: %f\n", diff);

				if(status.pkt_state[i][j] == VLC_unacked && diff<=(double)0){
	//						printf("Timeout after Received %d report(s)\n",after_n_report);
	//						after_n_report=0;
					//[TESTING]CHECKING IF CALCULATION OF TIMEOUT IS CORRECT
					//printf("--------TIMEOUT for packet node %d, seq number %d-----\n",i+1,j);

					//GET FROM UNACKED VLC ARRAY, SEND TO RF, REMOVE FROM ARRAY
					for(l=0;l<n_unacked_vlc_packet;l++){
						if(unacked_vlc_packet[l].node_id-1 == i && unacked_vlc_packet[l].seq_number == j){
							//IF PACKET FOUND, TRANSMIT VIA RF
							NODE *resend = (NODE*) malloc(sizeof(NODE));
							if(resend != NULL){
								resend->data = unacked_vlc_packet[l];

								pthread_mutex_lock(&rf_queue_mutex);
								push(rf_queue,resend);
								pthread_mutex_unlock(&rf_queue_mutex);

								status.pkt_state[i][j] = RF;
								r++;

								unacked_vlc_packet[l] = unacked_vlc_packet[n_unacked_vlc_packet-1];
								n_unacked_vlc_packet--;
								break;
							}
						}
					}
				}
			}
		}

		usleep(2000);
	}

	pthread_exit(NULL);
}

void *schedule_packet(void *tid){
	NODE *pN;
	int i,j,unsched_pkt_idx;
	struct scheduling_result sched_result;

	struct timespec current_time;
	int scheduled_from_vlc=0,scheduled_from_rf=0;

	//For Bandwidth checking
//	struct timeval t1, t2;
//	double elapsedTime;
//	int total_bytes = 0;
//	gettimeofday(&t1, NULL);

	for(;;){
		for(i=0;i<N_NODE;i++){
			sched_result = schedule(i);
			if(sched_result.mode != IDLE_MODE){
				NODE *temp;

				pthread_mutex_lock(&node_queue_mutex);
				temp = pop(node_queues[i]);
				pthread_mutex_unlock(&node_queue_mutex);

				unsched_pkt_idx = status.unsched_seq[i];
				status.unsched_seq[i] = (status.unsched_seq[i] + 1) % MAX_SEQ_NUMBER;

				if(temp != NULL){

					gn[i]--; //IF NodeQueue Popped Successfully, Decrease the gateway Queue size of Node

					if(sched_result.mode == RF_MODE){
						pthread_mutex_lock(&rf_queue_mutex);
						push(rf_queue,temp);
						pthread_mutex_unlock(&rf_queue_mutex);

						status.pkt_state[i][unsched_pkt_idx] = RF;
						scheduled_from_rf++;
						r++;
					}
					else{
						//MODE > 0 (SCHEDULED TO VLC)
						int* VLC_Tx_set = sched_result.vlc_tgt_set;
						int VLC_Tx_set_size = sched_result.n_vlc_tgt;
						for(j=0;j<VLC_Tx_set_size;j++){
							NODE *vlc_temp = temp;
							vlc_temp->data.vlc_id = VLC_Tx_set[j];
//								printf("VLC QUEUE SIZE: %d\n",vlc_queue_size);
//								vlc_mutex_lock = 1;
//								printf("before push vlc queue\n");
							push(vlc_queue,vlc_temp);

							d_vlc[VLC_Tx_set[j]]+= 1; //INCREASE THE DEPARTURE PACKET FOR RELATED VLC TX
						}

						free(sched_result.vlc_tgt_set); //Release the memory of VLC Target Set

//							printf("unsched_pkt_idx:%d\n",unsched_pkt_idx);
						status.pkt_state[i][unsched_pkt_idx] = VLC_unacked;

						scheduled_from_vlc++;
//							printf("scheduled from vlc: %d\n",scheduled_from_vlc);
//							//ADD TIMEOUT FROM CURRENT TIME
						clock_gettime(CLOCK_MONOTONIC,&current_time);
						current_time.tv_sec += TIMEOUT_SECOND;
						current_time.tv_nsec += TIMEOUT_NANOSECOND;
						status.expire_time[i][unsched_pkt_idx] = current_time;

						//ADD PACKET TO UNACKED VLC ARRAY
						if((n_unacked_vlc_packet+1) < MAX_UNACKED_VLC_PACKET){
							//IF the array that temporarily store unacked vlc packet is full, the packet is discarded
							unacked_vlc_packet[n_unacked_vlc_packet] = temp->data;
							n_unacked_vlc_packet++;
						}
					}
				}
			}

			//print_gateway_status(status,N_NODE,MAX_SEQ_NUMBER);
		}

		usleep(SCHEDULING_PERIOD);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]){
	int i,j,l, t; //t = epoch

	//INITIALIZATION
	// struct gateway_status initializeGatewayStatus; 임.
	status = initializeGatewayStatus(N_NODE,MAX_SEQ_NUMBER);
	//print_gateway_status(status,N_NODE,MAX_SEQ_NUMBER);
	setup_node_address(); // node 주소 설정
	setup_vlc_address(); // vlc 주소 설정
	setup_rf_socket(); // rf socket -> 소켓 프로그래밍 공부후에 다시 분석.
	setup_sending_vlc_packet(); // vlc socket -> 소켓 프로그래밍 공부
	initialize_algorithm_3_variables(); // algorithm_3_variables의 함수호출

	//QUEUE INITIALIZATION. QUEUE 초기설정.
	node_queues = (packet_queue**) malloc(N_NODE * sizeof(packet_queue *));
	for(i = 0;i<N_NODE;i++){
		// packet_queue.c에 코딩되어있고, packet_queue_h에 정의되어있다.
		// haed, tail, size, limit이 정의되어있다.
		node_queues[i] = construct_queue(NODE_QUEUE_SIZE);
	}

	rf_queue = construct_queue(RF_QUEUE_SIZE);
	vlc_queue = construct_queue(RF_QUEUE_SIZE);
	tbf_queue = construct_queue(LIMIT);

	//Alarm for scheduler
	// scheduler의 알람을 위해서 쓴 듯...
	signal(SIGALRM, handle_scheduler); // SIGALRM is ALACM CLOCK
	ualarm(SCHEDULING_PERIOD,0);

	// mutex는 쓰레드가 공유하는 데이터 영역을 보호하기 위해서 사용되는 도구이다.
	// pthread_mutex_init는 mutex 객체를 초기화할 때 사용한다.
	pthread_mutex_init(&node_queue_mutex, NULL);

	//Thread FOR REPORT RECEIVING AND RF TRANSMISSION
	pthread_t threads[9];
	// 첫번째 agrument는 성공적으로 생성되었을때, 생성된 쓰레드를 식별하기 위해서 사용되는 쓰레드 식별자.
	// 두번째는 특성 지정이며, 기본 쓰레드 특성을 이용하고자 할 경우에는 NULL이다.
	// 세번째는 분기시켜서 실행할 쓰레드 함수.
	// 네번째는 쓰레드 함수의 인자.
	if(pthread_create(&threads[RF_THREAD_ID],NULL,transmit_rf,(void *)RF_THREAD_ID)
			||pthread_create(&threads[REPORT_THREAD_ID],NULL,receive_report,(void *)REPORT_THREAD_ID)
			||pthread_create(&threads[TOKEN_ADDING_THREAD_ID],NULL,add_token,(void *)TOKEN_ADDING_THREAD_ID)
			||pthread_create(&threads[TBF_TRANSMIT_THREAD_ID],NULL,tbf_transmit,(void *)TBF_TRANSMIT_THREAD_ID)
			||pthread_create(&threads[VLC_QUEUE_THREAD_ID],NULL,receive_vlc_report,(void *)VLC_QUEUE_THREAD_ID)
			||pthread_create(&threads[VLC_THREAD_ID],NULL,transmit_vlc,(void *)VLC_THREAD_ID)
			||pthread_create(&threads[GENERATE_PACKET_ID],NULL,generate_packet,(void *)GENERATE_PACKET_ID)
//			||pthread_create(&threads[UNACKED_CHECK_ID],NULL,check_unacked,(void *)UNACKED_CHECK_ID)
			||pthread_create(&threads[SCHEDULE_PACKET_ID],NULL,schedule_packet,(void *)SCHEDULE_PACKET_ID)
			){
		printf("Error initializing thread\n");
		exit(-1);
	}



	printf("--INITIALIZATION FINISHED--\n");
	printf("Gateway Queue, RF Queue, VLC Queue, Scheduled to RF, Scheduled to VLC\n");

	//For Data Output
	struct timeval t1, t2;
	double elapsedTime;
	int total_bytes = 0;
	gettimeofday(&t1, NULL);

	for(i=0;i<9;i++){
		// 함수를 이용해서 쓰레드 종료할 때까지 기다려줘야함.
		// 쓰레드자원을 해제.
		pthread_join(threads[i],NULL);
		// i = RF_THREAD_ID, REPORT_THREAD_ID, TOKEN_ADDING_THREAD_ID, TBF_TRANSMIT_THREAD_ID
		// VLC_QUEUE_THREAD_ID, VLC_THREAD_ID, GENERATE_PACKET_ID, SCHEDULE_PACKET_ID 임.
		}


	close(sockfd); // 소켓 종료.
	return 0;
} // main함수 정료.

int transmit_packet_rf(GATEWAYPACKET data){
	int rf_bytes_sent,i;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	int vlc_id = 0;

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&rf_if_mac.ifr_hwaddr.sa_data)[5];
//	printf("Our Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5]);
	eh->ether_dhost[0] = NODE_MAC0;
	eh->ether_dhost[1] = NODE_MAC1;
	eh->ether_dhost[2] = NODE_MAC2;
	eh->ether_dhost[3] = NODE_MAC3;
	eh->ether_dhost[4] = NODE_MAC4;
	eh->ether_dhost[5] = NODE_MAC5;
//	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ALL);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	//CONSTRUCT THE PACKET DATA
	memcpy(&sendbuf[tx_len],(uint8_t*)&data.node_id,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(uint8_t*)&vlc_id,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(uint8_t*)&data.payload_len,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(unsigned char*)&data.seq_number,sizeof(uint32_t));
	tx_len+=sizeof(uint32_t);
	for(i=0;i<data.payload_len;i++){
		sendbuf[tx_len] = data.data[i];
		tx_len+=sizeof(uint8_t);
	}

	/* Index of the network device */
//	socket_address.sll_ifindex = eth_if_idx.ifr_ifindex;
	socket_address.sll_ifindex = 3;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = NODE_MAC0;
	socket_address.sll_addr[1] = NODE_MAC1;
	socket_address.sll_addr[2] = NODE_MAC2;
	socket_address.sll_addr[3] = NODE_MAC3;
	socket_address.sll_addr[4] = NODE_MAC4;
	socket_address.sll_addr[5] = NODE_MAC5;

	rf_bytes_sent = sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
	if (rf_bytes_sent < 0){
		printf("Send failed\n");
	}

	return rf_bytes_sent;
}

int transmit_packet_vlc(struct gateway_packet content){
	int eth_bytes_sent,i;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;

	int vlc_id = content.vlc_id;

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
	eh->ether_dhost[0] = VLC_MAC0;
	eh->ether_dhost[1] = VLC_MAC1;
	eh->ether_dhost[2] = VLC_MAC2;
	eh->ether_dhost[3] = VLC_MAC3;
	eh->ether_dhost[4] = VLC_MAC4;
	eh->ether_dhost[5] = VLC_MAC5;
//	printf("Destination Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_ALL);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	vlc_id += 1;
//	printf("------\nTransmitted Packet: \n");
//	print_gateway_packet(content);
//	printf("-----\n");
	memcpy(&sendbuf[tx_len],(uint8_t*)&content.node_id,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(uint8_t*)&vlc_id,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(uint8_t*)&content.payload_len,sizeof(uint16_t));
	tx_len+=sizeof(uint16_t);
	memcpy(&sendbuf[tx_len],(unsigned char*)&content.seq_number,sizeof(uint32_t));
	tx_len+=sizeof(uint32_t);
	for(i=0;i<content.payload_len;i++){
		sendbuf[tx_len] = content.data[i];
		tx_len+=sizeof(uint8_t);
	}

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
	eth_bytes_sent = sendto(eth_fd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
	if (eth_bytes_sent < 0){
		printf("Send failed\n");
	}


	return eth_bytes_sent;
}

// rf socket 설정.
int setup_rf_socket(){
	int sockopt;
	char rf_if_name[IFNAMSIZ]; //Name of Ethernet Interface
	strcpy(rf_if_name, RF_IF_NAME); // "ra0" 을 rf_if_name에 복사.

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("listener: socket");
		return -1;
	}

	memset(&rf_if_mac, 0, sizeof(struct ifreq));
	strncpy(rf_if_mac.ifr_name, rf_if_name, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &rf_if_mac) < 0)
		perror("SIOCGIFHWADDR");

	memset(&rf_if_idx, 0, sizeof(struct ifreq));
	strncpy(rf_if_idx.ifr_name, rf_if_name, IFNAMSIZ-1);
	if(ioctl(sockfd,SIOCGIFINDEX,&rf_if_idx) == -1){
//		printf("%s", strerror(errno));
		perror("SIOCGIFINDEX");
		return -1;
	}

		/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(sockfd);
//		exit(EXIT_FAILURE);
		return -1;
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, rf_if_name, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(sockfd);
//		exit(EXIT_FAILURE);
		return -1;
	}

	return 0;
}

int setup_sending_vlc_packet(){
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
