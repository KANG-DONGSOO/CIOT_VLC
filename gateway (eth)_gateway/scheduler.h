#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdio.h>
#include <stdlib.h>

#define IDLE_MODE -1
#define RF_MODE 0
#define VLC_MODE 1

struct scheduler{
	int mode;
};

struct scheduler intialize_scheduler();
int schedule(int node_id);
int* generate_vlc_tx_set(int node_id);
#endif
