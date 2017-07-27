#ifndef REQUESTTABLE_H
#define REQUESTTABLE_H
#define PATH_MAX 128
/*
* Holds RCB table and RCB node things
*/

// request control block
typedef struct {
  int sequenceNum; // sequence number
  int clientFD; //file descriptor
  FILE * handle;
  int numBytesRemaining; // number remain to be sent
  int quantum; // max number bytes to be sent
  char path[PATH_MAX]; // holds path for printing
} RCB;

// for making the list/queue
struct RCBnode {
  RCB* rcb;
  struct RCBnode* next;
};

// struct for scheduler
typedef struct {
  struct RCBnode * requestTable;
  int type;
} Scheduler;

// function for adding RCB to queue
void addRCBtoQueue(RCB* rcb, Scheduler* sched);

//function for adding RCB to queue for SJF
void addRCBtoQueueForSJF(RCB* rcb, Scheduler* sched);

// gets next request to process
RCB* getNextRCB(Scheduler* sched);

// inits the request table with malloc
void initRequestTable(Scheduler* sched);

#endif /* REQUESTTABLE_H */