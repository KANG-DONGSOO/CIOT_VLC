#include "scheduler.h"

struct scheduler intialize_scheduler(){
	struct scheduler temp;
	temp.mode = IDLE_MODE;

	return temp;
}

int schedule(int node_id){
	return rand()%3 -1; //Dummy before scheduling formula implemented
	//return VLC_MODE;
}

int* generate_vlc_tx_set(int node_id){
	int* temp = (int *) malloc(2 * sizeof(int));
	temp[0] = 0;
	temp[1] = 1;

	return temp;
}
