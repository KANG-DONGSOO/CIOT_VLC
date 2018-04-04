#include "status_report_packet.h"

struct status_report_packet generate_status_report_packet(int n_vlctx,int bit_pattern_length){
	struct status_report_packet temp;
	int i;

	temp.node_id = 0;
	temp.ACK_number = 0;
	temp.pattern_length = bit_pattern_length;
	temp.ACK_bit_pattern = (unsigned char *) malloc(bit_pattern_length * sizeof(unsigned char));
	temp.n_vlc = n_vlctx;
	temp.vlc_data = (struct vlc_information *) malloc(n_vlctx * sizeof(struct vlc_information));

	return temp;
}

char * convert_report_packet_to_string(struct status_report_packet packet, int vlc_tx_number,int* report_length){
	char* buff;
	int idx = 0, i,j, pattern_bytes;
	int unsigned_char_size,remaining_pattern_length,cur_idx,cur_last_idx,n_shift;

	buff = (unsigned char *) malloc(sizeof(unsigned char) * 1000);
	buff[idx++] = (unsigned char) (packet.node_id & 0xff);
	memcpy(&buff[idx],(unsigned char *) &packet.ACK_number,sizeof(int));
	idx+=sizeof(int);
	memcpy(&buff[idx],(unsigned char *) &packet.pattern_length,sizeof(int));
	idx+=sizeof(int);
	unsigned_char_size = sizeof(unsigned char) * 8;
	pattern_bytes = packet.pattern_length / unsigned_char_size + (packet.pattern_length % unsigned_char_size == 0 ? 0 : 1);

	remaining_pattern_length = packet.pattern_length;
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
			temp_byte = temp_byte | (packet.ACK_bit_pattern[j]& 0x1)<<n_shift; //Convert each character (0 or 1) in the packet bit pattern to a bit
			n_shift--;
		}
		//printf("pattern byte %d: %08x\n",i,temp_byte);
		buff[idx++] = temp_byte;
		remaining_pattern_length -= unsigned_char_size;
	}

	buff[idx++] = (unsigned char) (vlc_tx_number & 0xff);

	for(i=0;i<vlc_tx_number;i++){
		buff[idx++] = (unsigned char) (packet.vlc_data[i].vlc_tx_id & 0xff);
		buff[idx++] = (unsigned char) (packet.vlc_data[i].vlc_link_quality & 0xff);
	}

	*report_length = idx;
	return buff;
}

struct status_report_packet generate_dummy_report_for_testing(){
	struct status_report_packet temp;
	int i,idx=0;

	temp.node_id = 1;
	temp.ACK_number = 199720;
	temp.pattern_length = 24;
	temp.ACK_bit_pattern = (unsigned char *) malloc(temp.pattern_length * sizeof(unsigned char));
	temp.ACK_bit_pattern[idx++] = '1';
	temp.ACK_bit_pattern[idx++] = '1';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '1';

	temp.ACK_bit_pattern[idx++] = '1';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '1';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';

	temp.ACK_bit_pattern[idx++] = '1';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';
	temp.ACK_bit_pattern[idx++] = '0';

	temp.n_vlc = 1;
	temp.vlc_data = (struct vlc_information *) malloc(temp.n_vlc * sizeof(struct vlc_information));

	temp.vlc_data[0].vlc_tx_id = 1;
	temp.vlc_data[0].vlc_link_quality = 100;

	return temp;
}
