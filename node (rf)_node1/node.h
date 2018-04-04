#ifndef NODE_H
#define NODE_H

#include <arpa/inet.h>
#include <errno.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "node_packet.h"
#include "packet_queue.h"
#include "status_report_packet.h"

//CONFIGURATION
#define OUTPUT_PERIOD 50000
#define VLC_Q_PERIOD 10000
#define REPORT_PERIOD 500
#define SELF_ID 1
#define N_VLC 1
#define ALPHA 0.05f

#define MAX_SEQ_NUMBER 500000
#define MAXREPORTLEN 100
#define MAXBUFLEN 1024
#define MYPORT "4950"
#define max_ACK_bit_pattern 100000
#define MAX_PROCESS_QUEUE 1000000

#define TFRAME 10000

#define VLC_INTERFACE_NAME "vlc0"
#define RF_INTERFACE_NAME "ra0"
#define BUF_SIZE 1024

#define N_THREAD 6

#define RF_IF_IDX 3

#define RCV_VLC_TID 0
#define RCV_RF_TID 1
#define PROC_DATA_TID 2
#define VLC_Q_TID 3
#define OUTPUT_TID 4
#define SEND_REPORT_TID 5

#define DUMMY_PACKET_NODE_ID 99
#define RS_ERROR_ID 98
#define PAYLOAD_LEN 74

#define MTU_VALUE 100

//Structure to manage the status of the received packets
struct node_status {
	int rcv_seq;
	int *rcv_state;
	int last_rcv_seq;
};


struct node_status initialize_node_status(int max_sequence);
void print_node_status(struct node_status status, int max_sequence);
#endif
