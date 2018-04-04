#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H
#include <stdlib.h>
#include <stdio.h>

#include "gateway_packet.h"
#define TRUE  1
#define FALSE	0

typedef struct Node_t {
    GATEWAYPACKET data;
    struct Node_t *prev;
} NODE;

/* the HEAD of the Queue, hold the amount of node's that are in the queue*/
typedef struct Queue {
    NODE *head;
    NODE *tail;
    int size;
    int limit;
} Queue;

Queue *ConstructQueue(int limit);
void DestructQueue(Queue *queue);
int Enqueue(Queue *pQueue, NODE *item);
NODE *Dequeue(Queue *pQueue);
int isEmpty(Queue* pQueue);

#endif
