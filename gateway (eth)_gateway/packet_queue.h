#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H
#include <stdlib.h>
#include <stdio.h>

#include "gateway_packet.h"

#define TRUE 1
#define FALSE 0

typedef struct Node_t {
    GATEWAYPACKET data;
    struct Node_t *next;
} NODE;

/* the HEAD of the Queue, hold the amount of node's that are in the queue*/
typedef struct Queue {
    NODE *head;
    NODE *tail;
    int size;
    int limit;
} packet_queue;

packet_queue *construct_queue(int limit);
void destruct_queue(packet_queue *queue);
int push(packet_queue *pQueue, NODE *item);
NODE *pop(packet_queue *pQueue);
int is_empty(packet_queue* pQueue);
void print_queue(packet_queue* queue);

#endif
