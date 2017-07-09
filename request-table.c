#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "request-table.h"

// TODO - make sure to free everything when we don't put it back in the queue
void initRequestTable(Scheduler* sched) {
  sched->requestTable = (struct RCBnode*)malloc(sizeof(struct RCBnode));
}

// function for adding RCB to queueu
void addRCBtoQueue(RCB* rcb, Scheduler* sched){
  struct RCBnode* next;
  struct RCBnode* node;
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
    next->rcb = rcb;
    sched->requestTable->next = next;
  }
    
}

// gets next request to process
RCB* getNextRCB(Scheduler* sched){
  if (sched == NULL || sched->requestTable == NULL || sched->requestTable->rcb == NULL) {
        return NULL;
  }
  RCB * next = sched->requestTable->rcb;
  struct RCBnode* node = sched->requestTable;
    
  sched->requestTable = sched->requestTable->next;
  free(node);
  return next;
}