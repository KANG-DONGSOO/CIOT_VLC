#ifndef GATEWAY_H
#define GATEWAY_H

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
//#include <linux/sockios.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/if_ether.h>
//#include <netinet/in.h>
#include <netpacket/packet.h>
// Posix Thread. 스레드들이 병력적으로 수행될 수 있도록
// 스레드를 새로 만들고 또 스케쥴링 하는 api.
// 유닉스 계열 운영체제(리눅스)에서 이용되는 라이브러리.
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h> // 소켓 Lib
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "gateway_packet.h"
#include "packet_queue.h"
#include "status_report_packet.h"
//#include "scheduler.h"

//CONFIGURATION
#define SCHEDULING_PERIOD 1320 //IN MICRO SECOND
#define TIMEOUT_SECOND 0
#define TIMEOUT_NANOSECOND 600000
#define N_NODE 1
#define N_VLC 1
#define NODE_QUEUE_SIZE 1000000
#define RF_QUEUE_SIZE 300000
#define MYPORT "4950"    // the port users will be connecting to
#define MAXBUFLEN 1000
#define VLC_BUF_SIZE 1024
#define SEND_BUFFER_SIZE 1000
#define PAYLOAD_SIZE 74
//#define N_MICROSECOND_PACKET_GENERATION 203
//#define GENERATED_PACKET_PER_N_MICROSECOND 12
#define N_MICROSECOND_PACKET_GENERATION 2600
#define GENERATED_PACKET_PER_N_MICROSECOND 1

//TBF PARAMETERS
#define TOKEN_RATE 1 // PACKET/SECOND (CAN BE CHANGED TO BYTE/SECOND)
#define TOKEN_PERIOD 2640
#define LIMIT 1 //NUMBER OF PACKET CAN EXIST IN TBF QUEUE
#define BURST 6 //MAXIMUM NUMBER OF TOKEN


#define MAXPACKETLEN 1024
#define VLC_INTERFACE_NAME "eth0"
#define RF_INTERFACE_NAME "ra0"
#define MAX_UNACKED_VLC_PACKET 500000
#define MAX_SEQ_NUMBER 500000

#define RF_THREAD_ID 0
#define REPORT_THREAD_ID 1
#define VLC_QUEUE_THREAD_ID 2
#define VLC_THREAD_ID 3
#define TOKEN_ADDING_THREAD_ID 4
#define TBF_TRANSMIT_THREAD_ID 5
#define GENERATE_PACKET_ID 6
#define UNACKED_CHECK_ID 7
#define SCHEDULE_PACKET_ID 8


#define VLC_unacked 1
#define VLC_acked 2
#define RF 3

#define IDLE_MODE -1
#define RF_MODE 0
#define VLC_MODE 1

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
#define RF_IF_NAME	"ra0" // Ethernet Interface 이름 설정.
#define BUF_SIZ		1024

//Structure to define the status of packet transmission
struct gateway_status {
	int* unacked_seq; // 첫번째 packet의 Sequence number, 노드n에 인식되지않은.
	int* unsched_seq; // 첫번째 packet의 sequence number, 노드n에 scheduling되지않은.
	int* arr_seq; // 다음에 올 노드n packet의 Sequence number.
	int** pkt_state; // packet(n,i)의 상태. 상태는 VLC_unacked / VLC_acked / RF가 될 수있다.
	struct timespec** expire_time; // VLC link에서 packet(n,i)가 손실된것으로 간주되는 만료시간. VLC_unacked상태인 packet일때만 유효.
};

struct scheduling_result {
	int mode;
	int* vlc_tgt_set;
	int n_vlc_tgt;
};

struct node_address{
	char *ip_address;
	char *port;
};

struct gateway_status initializeGatewayStatus(int node, int maxSeqNumber);
void print_gateway_status(struct gateway_status temp, int node, int max_seq_number);
void *get_in_addr(struct sockaddr *sa);
int generate_m(int *vlc_ids, int n_vlc);
int is_k_in_sn(int vlc_id,int sn);
#endif
