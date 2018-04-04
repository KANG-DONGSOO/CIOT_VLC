#include "gateway_packet.h"

struct gateway_packet generate_gateway_packet(uint16_t node_id, uint16_t vlc_id, uint16_t payload_len, uint32_t seq_number, uint8_t* data){
	struct gateway_packet temp;
	temp.node_id = node_id;
	temp.vlc_id = vlc_id;
	temp.payload_len = payload_len;
	temp.seq_number = seq_number;
	temp.data = data;
	return temp;
}

void print_gateway_packet(GATEWAYPACKET packet){
	int i;
	printf("Packet Node [%d]\nVLC ID: [%d]\nPayload Len: %d\nSequence [%d]\nData: ",packet.node_id,packet.vlc_id,packet.payload_len,packet.seq_number);
	for(i=0;i<packet.payload_len;i++){
		printf("%u ",packet.data[i]);
	}
	printf("\n");
}
