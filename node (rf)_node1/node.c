#include "node.h"

struct node_status initialize_node_status(int max_sequence){
	struct node_status temp;
	int i;

	temp.rcv_seq = 0;
	temp.rcv_state = (int *) malloc (max_sequence * sizeof(int));
	for(i=0;i<max_sequence;i++)
		temp.rcv_state[i]  = 0;
	temp.last_rcv_seq = 0;

	return temp;
}

void print_node_status(struct node_status status, int max_sequence){
	int i;

	printf("First Packet that not received: %d\n",status.rcv_seq);
	printf("Receive state of packet:\n");
	for(i=0;i<max_sequence;i++){
		printf("%d ",status.rcv_state[i]);
	}
	printf("\n");
	printf("Received packet with the largest sequence number: %d\n\n",status.last_rcv_seq);
}
