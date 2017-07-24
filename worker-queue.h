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



// for making the list/queue
struct WorkerNode {
    pthread_t* thread;
    struct WorkerNode* next;
};

/* functions */
void addWorkerToQueue(struct WorkerNode* add, struct WorkerNode* front);
struct WorkerNode* createWorkerNode(pthread_t* thread);
struct WorkerNode* popFrontWorkerQueue(struct WorkerNode** front);