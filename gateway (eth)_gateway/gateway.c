#include "gateway.h"

struct gateway_status initializeGatewayStatus(int node, int maxSeqNumber){
	struct gateway_status temp;
	int i,j;

	temp.unacked_seq = (int *) malloc(node * sizeof(int));
	temp.unsched_seq = (int *) malloc(node * sizeof(int));
	temp.arr_seq = (int *) malloc(node * sizeof(int));
	temp.pkt_state = (int **) malloc(node * sizeof(int*));

	for(i=0;i<node;i++){
		temp.pkt_state[i] = (int *) malloc(maxSeqNumber * sizeof(int));
	}
	temp.expire_time = (struct timespec **) malloc(node * sizeof(struct timespec*));
	for(i=0;i<node;i++){
		temp.expire_time[i] = (struct timespec *) malloc(maxSeqNumber * sizeof(struct timespec));
	}


	for(i =0;i<node;i++){
		temp.unacked_seq[i] = 0;
	}

	for(i=0;i<node;i++){
		temp.unsched_seq[i] = 0;
	}

	for(i=0;i<node;i++){
		temp.arr_seq[i] = 0;
	}

	for(i=0;i<node;i++){
		for(j=0;j<maxSeqNumber;j++){
			temp.pkt_state[i][j] = 0;
		}
	}

	for(i = 0;i<node;i++){
		struct timespec now;
		//ZERO VALUE FOR INITIALIZATION
		now.tv_sec = (time_t) 0;
		now.tv_nsec = (long) 0;

		for(j=0;j<maxSeqNumber;j++){
			temp.expire_time[i][j] = now; //CURRENT TIME
		}
	}

	return temp;
}

void print_gateway_status(struct gateway_status temp, int node, int max_seq_number){
	int i,j;

	printf("Unacked Sequence:\n");
	for(i =0;i<node;i++){
		printf("%d ",temp.unacked_seq[i]);
	}

	printf("\nUnscheduled Sequence:\n");
	for(i=0;i<node;i++){
		printf("%d ",temp.unsched_seq[i]);
	}

	printf("\nNext Arrival Sequence:\n");
	for(i=0;i<node;i++){
		printf("%d ",temp.arr_seq[i]);
	}

	printf("\nPacket State:\n");
	for(i=0;i<node;i++){
		printf("Node %d : ",i);
		for(j=0;j<max_seq_number;j++){
			printf("%d ",temp.pkt_state[i][j]);
		}
		printf("\n");
	}

	printf("\nExpired Time for each nodes packets:\n");
	for(i = 0;i<node;i++){
		printf("Node %d: ",i);

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC,&now);
		uint64_t diff;

		for(j=0;j<max_seq_number;j++){
			diff = 1000000000L * (temp.expire_time[i][j].tv_sec - now.tv_sec) + temp.expire_time[i][j].tv_nsec - now.tv_nsec;
			if(diff >0)
				printf("%.llu ",diff);
			else
				printf("TO ");
		}
		printf("\n");
	}

	printf("\n");
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int generate_m(int *vlc_ids, int n_vlc){
	int i;
	int retval = 0;

	for(i=0;i<n_vlc;i++){
		int temp = 1;
		temp = temp<<(vlc_ids[i]-1);
		printf("temp: %d\n",temp);
		retval = retval | temp;
		printf("retval: %d\n\n",retval);
	}

	return retval;
}

int is_k_in_sn(int vlc_id,int sn){
	if(sn<=0)
		return 0;

	return ((1<<(vlc_id-1))&sn) > 0;
}
