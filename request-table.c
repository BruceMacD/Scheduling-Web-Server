#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "request-table.h"


pthread_mutex_t requesttable_lock = PTHREAD_MUTEX_INITIALIZER;

void initRequestTable(Scheduler* sched) {
    sched->requestTable = (struct RCBnode*)malloc(sizeof(struct RCBnode));
    sched->requestTable->next = NULL;
}

// function for adding RCB to queue
void addRCBtoQueue(RCB* rcb, Scheduler* sched){

    struct RCBnode* next = NULL;
    struct RCBnode* node = NULL;

    //check lock
    if( pthread_mutex_lock( &requesttable_lock ) ) abort();

    if (sched->requestTable == NULL) {               // the queue is empty, so make a new node

        initRequestTable(sched);
        node = sched->requestTable;      // we want to get the request table
        node->rcb = rcb;
    } else {

        node = sched->requestTable;      // we want to get the request table
        // if queue is empty, add to from
        while (node->next != NULL) {
            node = node->next;
        }
        next = (struct RCBnode*)malloc(sizeof(struct RCBnode));
        next->next = NULL;
        next->rcb = rcb;
        node->next = next;
    }

    pthread_mutex_unlock( &requesttable_lock );
}

void addRCBtoQueueForSJF(RCB* rcb, Scheduler* sched){

    //critical section, only one thread at a time can do this
    struct RCBnode* next = NULL;
    struct RCBnode* node = NULL;
    int value;

    //check lock
    if( pthread_mutex_lock( &requesttable_lock ) ) abort();

    //Add node to empty list
    if (sched->requestTable == NULL) {
        initRequestTable(sched);
        node = sched->requestTable;
        node->next = NULL;
        node->rcb = rcb;
    }
    //Add node to list based on increasing order of numBytesRemaining to be processed
    else {
        node = sched->requestTable;
        value = rcb->numBytesRemaining;
        //Add node to front of list
        if(node->rcb->numBytesRemaining > value){
            next = (struct RCBnode*)malloc(sizeof(struct RCBnode));
            next->next = NULL;
            next->rcb = node->rcb;
            next->next = node->next;
            sched->requestTable->rcb = rcb;
            sched->requestTable->next = next;
        }
        //Add node between two nodes
        else{
            while (node->next != NULL && node->next->rcb->numBytesRemaining < value) {
                node = node->next;
            }
            next = (struct RCBnode*)malloc(sizeof(struct RCBnode));
            next->next = NULL;
            next->rcb = rcb;
            next->next = node->next;
            node->next = next;
        }
    }

    pthread_mutex_unlock( &requesttable_lock );
}

// gets next request to process
RCB* getNextRCB(Scheduler* sched){
    if (sched == NULL || sched->requestTable == NULL || sched->requestTable->rcb == NULL) {
        return NULL;
    }
    RCB * next = sched->requestTable->rcb;
    if (sched != NULL && sched->requestTable != NULL){
        free(sched->requestTable);
    }
    sched->requestTable = sched->requestTable->next;

    return next;
}