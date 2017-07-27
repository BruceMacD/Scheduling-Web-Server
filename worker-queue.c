
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "network.h"
#include "request-table.h"
#include "worker-queue.h"

pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

/* add the workers to the worker queue */
void addWorkerToQueue(struct WorkerNode* add, struct WorkerNode** front)
{
    // this is all critical section, only one thread should do it at a time
    struct WorkerNode* node = *front;

    //check lock
    if( pthread_mutex_lock( &queue_lock ) ) abort();
    
    if (node == NULL) {
        *front = add;
        pthread_mutex_unlock( &queue_lock );
        return;
    }
    
    if (add == NULL) {
        pthread_mutex_unlock( &queue_lock );
        return;
    }
    
    
    while (node->next != NULL) {
        node = node->next;
    }
    node->next = add;
    pthread_mutex_unlock( &queue_lock );
}

// instantiate a new node
struct WorkerNode* createWorkerNode(RCB *rcb) {
    struct WorkerNode* node = (struct WorkerNode*)malloc(sizeof(struct WorkerNode));
    node->rcb = rcb;
    node->next = NULL;
    return node;
}

// pop front front
struct WorkerNode* popFrontWorkerQueue(struct WorkerNode** front) {
    
    // this is all critical section, only one thread should do it at a time
    //check lock
    if( pthread_mutex_lock( &queue_lock ) ) abort();

    if (front == NULL || (*front) == NULL) {
        pthread_mutex_unlock( &queue_lock );
        return NULL;
    }

    RCB * rcb = (*front)->rcb;
    struct WorkerNode* new = createWorkerNode(rcb);
    
    free(*front);
    (*front) = (*front)->next;
    
    pthread_mutex_unlock( &queue_lock );
    return new;
}

