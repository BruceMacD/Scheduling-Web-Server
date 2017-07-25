
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "network.h"
#include "request-table.h"
#include "worker-queue.h"

/* add the workers to the worker queue */
void addWorkerToQueue(struct WorkerNode* add, struct WorkerNode* front)
{
    struct WorkerNode* node = front;
    
    if (node == NULL) {
        front = add;
        return;
    }
    
    if (add == NULL) {
        return;
    }
    
    while (node->next != NULL) {
        node = node->next;
    }
    node->next = add;
}

// instantiate a new node
struct WorkerNode* createWorkerNode(Scheduler sched) {
    struct WorkerNode* node = (struct WorkerNode*)malloc(sizeof(struct WorkerNode));
    node->sched = sched;
    node->next = NULL;
    return node;
}

// pop front front
struct WorkerNode* popFrontWorkerQueue(struct WorkerNode** front) {
    
    Scheduler* s = (*front)->sched;
    struct WorkerNode* new = createWorkerNode(s);
    
    //struct WorkerNode* node = front;
    free(*front);
    (*front) = (*front)->next;
    
    
    return new;
}