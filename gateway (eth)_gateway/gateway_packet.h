#ifndef GATEWAY_PACKET_H
#define GATEWAY_PACKET_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//Structure of the packet when it is received from internet
typedef struct gateway_packet {
	uint16_t node_id;
	uint16_t vlc_id;
	uint16_t payload_len;
	uint32_t seq_number;
	uint8_t *data;
} GATEWAYPACKET;


struct gateway_packet generate_gateway_packet(uint16_t node_id, uint16_t vlc_id, uint16_t payload_len, uint32_t seq_number, uint8_t* data);
void print_gateway_packet(GATEWAYPACKET packet);
#endif
