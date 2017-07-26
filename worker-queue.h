#ifndef WORKQUEUE_H
#define WORKQUEUE_H
/* worker queue stuff

 run like this, first make a front node, then add susequent nodes
 
pthread_t* p;
struct WorkerNode* workerQueue = createWorkerNode(p);

struct WorkerNode* newnode1 = createWorkerNode(p);
struct WorkerNode* newnode2 = createWorkerNode(p);

addWorkerToQueue(newnode1, workerQueue);
addWorkerToQueue(newnode2, workerQueue);
 */


#include <pthread.h>
#include "request-table.h"



// for making the list/queue
struct WorkerNode {
    RCB *rcb;
    struct WorkerNode* next;
};

// this gets passed to the threads
typedef struct {
    struct WorkerNode* workerQueue;
    Scheduler* sched;
}WorkerThreadData;

/* functions */
void addWorkerToQueue(struct WorkerNode* add, struct WorkerNode** front);
struct WorkerNode* createWorkerNode(RCB *rcb);
struct WorkerNode* popFrontWorkerQueue(struct WorkerNode** front);

#endif /* WORKQUEUE_H */