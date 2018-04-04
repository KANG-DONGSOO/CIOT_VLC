#include "status_report_packet.h"

struct status_report_packet generate_status_report_packet(int n_vlctx,int bit_pattern_length){
	struct status_report_packet temp;
	int i;

	temp.node_id   = 0;
	temp.ACK_number = 0;
	temp.pattern_length = bit_pattern_length;
	temp.ACK_bit_pattern = (unsigned char *) malloc(bit_pattern_length * sizeof(unsigned char));
	temp.n_vlc = n_vlctx;
	temp.vlc_data = (struct vlc_information *) malloc(n_vlctx * sizeof(struct vlc_information));

	return temp;
}

struct status_report_packet generate_status_report_from_string(unsigned char * report_bytes){
	struct status_report_packet temp;
	int i,j,idx = 0;
	int unsigned_char_size,pattern_bytes,convert_idx = 0,conv_remaining_pattern,conv_shift_until;

	temp.node_id = (int)report_bytes[idx++];
//	printf("node id: %d\n",temp.node_id);
	memcpy((unsigned char *)&temp.ACK_number,&report_bytes[idx],sizeof(int));
//	printf("ACK Number: %d\n",temp.ACK_number);
	idx+=sizeof(int);
	memcpy((unsigned char *)&temp.pattern_length,&report_bytes[idx],sizeof(int));
//	printf("Pattern length: %d\n",temp.pattern_length);
	idx+=sizeof(int);

	temp.ACK_bit_pattern = (unsigned char *) malloc(temp.pattern_length * sizeof(unsigned char));
	unsigned_char_size = sizeof(unsigned char) * 8;
	pattern_bytes = temp.pattern_length / unsigned_char_size + (temp.pattern_length % unsigned_char_size == 0 ? 0 : 1);
	conv_remaining_pattern = temp.pattern_length;

	for(i=0;i<pattern_bytes;i++){
		unsigned char byte_temp = report_bytes[idx++];
		if(conv_remaining_pattern >= unsigned_char_size){
			conv_shift_until = 0;
		}
		else{
			conv_shift_until  = unsigned_char_size - conv_remaining_pattern;
		}
		for(j=(unsigned_char_size-1);j>=conv_shift_until;j--){
			temp.ACK_bit_pattern[convert_idx++] = (unsigned char) byte_temp >>j & 0x1;
		}
		conv_remaining_pattern-=unsigned_char_size;
	}

	temp.n_vlc = (int) report_bytes[idx++];
	//printf("VLC TX Number: %d\n",vlc_tx_number);
	temp.vlc_data = (struct vlc_information *) malloc(temp.n_vlc * sizeof(struct vlc_information));
	for(i=0;i<temp.n_vlc;i++){
		temp.vlc_data[i].vlc_tx_id = (int) report_bytes[idx++];
		temp.vlc_data[i].vlc_link_quality = (int) report_bytes[idx++];
		//printf("VLC TX ID: %d, VLC LINK QUALITY: %d \n",temp.vlc_data[i].vlc_tx_id,temp.vlc_data[i].vlc_link_quality);
	}

	return temp;
}

char * convert_report_packet_to_string(struct status_report_packet packet){
	char* buff;
	int idx = 0;

	/*buff = (unsigned char *) malloc(sizeof(unsigned char) * report_byte_len);
	packet.ACK_number = 23;
	buff[idx] = (unsigned char) ((packet.ACK_number>>8) & 0xff); idx++;
	buff[idx] = (unsigned char) (packet.ACK_number & 0xff); idx++;*/

	return "dummy";
}

void print_report_content(struct status_report_packet packet){
	int i;

	printf("---------------\n");
	printf("Node ID: %d\n",packet.node_id);
	printf("ACK Number: %d\n",packet.ACK_number);
	printf("Pattern_length: %d\n",packet.pattern_length);
	printf("ACK Bit Pattern:\n");
	for(i=0;i<packet.pattern_length;i++){
		printf("%u ",packet.ACK_bit_pattern[i]);
	}
	printf("\nN VLC-Tx: %d\n",packet.n_vlc);
	for(i=0;i<packet.n_vlc;i++){
		printf("Quality for VLC-Tx%d: %d\n",packet.vlc_data[i].vlc_tx_id,packet.vlc_data[i].vlc_link_quality);
	}
	printf("---------------\n");
}
