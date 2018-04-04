#ifndef STATUSREPORTPACKET_H
#define STATUSREPORTPACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define report_byte_len 100

struct vlc_information {
	int vlc_tx_id;
	int vlc_link_quality;
};

struct status_report_packet{
	int node_id;
	int ACK_number;
	int pattern_length;
	unsigned char *ACK_bit_pattern;
	int n_vlc;
	struct vlc_information *vlc_data;
};


struct status_report_packet generate_status_report_packet(int n_vlctx,int bit_pattern_length);
char * convert_report_packet_to_string(struct status_report_packet packet, int vlc_tx_number,int* report_length);
struct status_report_packet generate_dummy_report_for_testing();

#endif
