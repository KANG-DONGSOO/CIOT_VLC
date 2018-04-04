#ifndef VLCTX_H
#define VLCTX_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "packet_queue.h"

//CONFIGURATION
#define REPORT_PERIOD 250	// In Microseconds
#define SCHEDULING_PERIOD 125 //IN MICRO SECOND
//#define TRANSMIT_PERIOD 10 //In Microseconds
#define N_NODE 1
#define QUEUE_LIMIT 1000000
#define IF_IDX_GATEWAY_ETH 2

#define SELF_ID 1

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

#define VLC_INTERFACE "vlc0"

#define ETH_IF_NAME	"eth0"
#define GATEWAY_ETH_IF_IDX 2
#define BUF_SIZ		1024

#define VLC_TRANSMIT_THREAD_ID 0
#define QUEUE_REPORT_THREAD_ID 1

#endif
