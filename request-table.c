#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "request-table.h"

// TODO - make sure to free everything when we don't put it back in the queue
void initRequestTable(Scheduler* sched) {
  sched->requestTable = (struct RCBnode*)malloc(sizeof(struct RCBnode));
}

// function for adding RCB to queue
void addRCBtoQueue(RCB* rcb, Scheduler* sched){
  struct RCBnode* next;
  struct RCBnode* node;
  if (sched->requestTable == NULL) {               // the queue is empty, so make a new node
    printf("First node \n");
    initRequestTable(sched);
    node = sched->requestTable;      // we want to get the request table
    node->rcb = rcb;
  } else {
    printf("Adding node to end \n");
    node = sched->requestTable;      // we want to get the request table
    // if queue is empty, add to from
    while (node->next != NULL) {
      node = node->next;
    }
    next = (struct RCBnode*)malloc(sizeof(struct RCBnode));
    next->rcb = rcb;
    node->next = next;
  }    
}

void addRCBtoQueueForSJF(RCB* rcb, Scheduler* sched){
    struct RCBnode* next;
    struct RCBnode* node;
    int value;
    //Add node to empty list
    if (sched->requestTable == NULL) {               
	printf("First node \n");
        initRequestTable(sched);
        node = sched->requestTable;     
        node->rcb = rcb;
    } 
    //Add node to list based on increasing order of numBytesRemaining to be processed
    else {
        node = sched->requestTable;      
		value = rcb->numBytesRemaining;
		next = (struct RCBnode*)malloc(sizeof(struct RCBnode));		//New node being added to the list
		//Add node to front of list
		if(node->rcb->numBytesRemaining > value){
			printf("Adding node to front \n");
			next->rcb = node->rcb;								//Copy current front data to the new front
			next->next = node->next;
			sched->requestTable->rcb = rcb;						//Old front of the list now has the data 
			sched->requestTable->next = next;					//And is pointed to by new front.
		}
		//Add node between two nodes
		else{
			 printf("Adding Node\n");
			 while (node->next->rcb->numBytesRemaining < value) {	//Find node who's Next is greater than our new node
				node = node->next;
			 }
			 next->rcb = rcb;									//New node has new data
			 next->next = node->next;							//New Node points to node with greater numBytesRemaining
			 node->next = next;    								//Current node points to new node.
		}   
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
