#include "node_packet.h"

struct node_packet generate_node_packet(uint16_t node_id, uint16_t vlc_id, uint16_t payload_len, uint32_t seq_number, uint8_t* data){
	struct node_packet temp;
	temp.node_id = node_id;
	temp.vlc_id = vlc_id;
	temp.payload_len = payload_len;
	temp.seq_number = seq_number;
	temp.data = data;

	return temp;
}

void print_node_packet(struct node_packet packet){
	int i;
	printf("Packet Node [%d]\nPayload Len: %d\nSequence [%d]\nData: ",packet.node_id,packet.payload_len,packet.seq_number);
	for(i=0;i<packet.payload_len;i++){
		printf("%u ",packet.data[i]);
	}
	printf("\n");
}

struct node_packet generate_from_bytes(unsigned char *buff){
	struct node_packet temp;
	int idx;

	temp.payload_len = ((uint16_t)buff[1]<<8) + (uint16_t)buff[0];
	temp.node_id = ((uint16_t)buff[3]<<8) + (uint16_t)buff[2];
	temp.vlc_id = ((uint16_t)buff[5]<<8) + (uint16_t)buff[4];
	idx = 3 * sizeof(uint16_t);
	memcpy((unsigned char*)&temp.seq_number,&buff[idx],sizeof(uint32_t));
	idx += sizeof(uint32_t);
	temp.data = (uint8_t *) malloc(temp.payload_len * sizeof(uint8_t));
	memcpy((unsigned char*)temp.data,&buff[idx],temp.payload_len * sizeof(uint8_t));

	return temp;
}
