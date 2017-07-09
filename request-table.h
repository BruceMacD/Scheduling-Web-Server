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
} RCB;

// for making the list/queue
struct RCBnode {
  RCB* rcb;
  struct RCBnode* next;
};

// struct for scheduler
typedef struct {
  struct RCBnode * requestTable;
} Scheduler;

// function for adding RCB to queueu
void addRCBtoQueue(RCB* rcb, Scheduler* sched);

// gets next request to process
RCB* getNextRCB(Scheduler* sched);

// inits the request table with malloc
void initRequestTable(Scheduler* sched);