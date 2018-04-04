#include "packet_queue.h"

packet_queue *construct_queue(int limit) {
    packet_queue *queue = (packet_queue*) malloc(sizeof (packet_queue));
    if (queue == NULL) {
        return NULL;
    }
    if (limit <= 0) {
        limit = 65535;
    }
    queue->limit = limit;
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;

    return queue;
}

void destruct_queue(packet_queue *queue) {
    NODE *pN;
    while (!is_empty(queue)) {
        pN = pop(queue);
        free(pN);
    }
    free(queue);
}

int push(packet_queue *pQueue, NODE *item) {
    /* Bad parameter */
	printf("debug");
	if ((pQueue == NULL) || (item == NULL)) {
        printf("bad parameter");
    	return FALSE;
    }
    // if(pQueue->limit != 0)
    if (pQueue->size >= pQueue->limit) {
        printf("limit");
    	return FALSE;
    }
    /*the queue is empty*/
    item->next = NULL;
    if (pQueue->size == 0) {
        pQueue->head = item;
        pQueue->tail = item;

    } else {
        /*adding item to the end of the queue*/
        pQueue->tail->next = item;
        pQueue->tail = item;
    }

    pQueue->size++;
    return TRUE;
}

NODE * pop(packet_queue *pQueue) {
    /*the queue is empty or bad param*/
    NODE *item;
    if (is_empty(pQueue))
        return NULL;
    item = pQueue->head;
    pQueue->head = (pQueue->head)->next;
    pQueue->size--;
    return item;
}

int is_empty(packet_queue* pQueue) {
    if (pQueue == NULL) {
        return FALSE;
    }
    if (pQueue->size == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void print_queue(packet_queue* queue){
	int i;

	NODE* temp = queue->head;
	printf("Packet Queue in gateway([idx] node seq_number)\n[0]: %d %d \n",temp->data.node_id,temp->data.seq_number);
	for(i=1;i<queue->size;i++){
		temp = temp->next;
		printf("[%d]: %d %d \n\n",i,temp->data.node_id,temp->data.seq_number);
	}

	printf("\n");
}
