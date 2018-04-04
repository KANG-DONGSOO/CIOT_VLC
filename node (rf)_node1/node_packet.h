#ifndef NODEPACKET_H
#define NODEPACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct node_packet {
	uint16_t node_id;
	uint16_t vlc_id;
	uint16_t payload_len;
	uint32_t seq_number;
	uint8_t *data;
} NODEPACKET;


struct node_packet generate_node_packet(uint16_t node_id, uint16_t vlc_id, uint16_t payload_len, uint32_t seq_number, uint8_t* data);
void print_node_packet(struct node_packet packet);
struct node_packet generate_from_bytes(unsigned char *buffer);

#endif
