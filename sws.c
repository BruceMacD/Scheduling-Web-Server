/*
 * File: sws.c
 * Author: Alex Brodsky
 * Purpose: This file contains the implementation of a simple web server.
 *          It consists of two functions: main() which contains the main
 *          loop accept client connections, and serve_client(), which
 *          processes each client request.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "network.h"
#include "request-table.h"
#include "worker-queue.h"

#define MAX_HTTP_SIZE_8KB 8192
#define MAX_HTTP_SIZE_64KB 65536                 /* size of buffer to allocate */

/* Definitions used to identify the scheduler during processing */
#define SJF 1
#define RR 2
#define MLFB 3

// global sequence counter
int g_sequenceCounter = 1;

//Scheduler Flags
int type_SJF = 0;
int type_RR = 0;
int type_MLFB = 0;

//The schedulers
//SJF scheduler
Scheduler schedSJF;
//round robin
Scheduler schedRR;
//high priority
Scheduler schedHIGH;
//medium priority
Scheduler schedMED;

WorkerThreadData workerThreadData;         /* what goes to pthreads */
//the worker queue
struct WorkerNode* workerQueue;



/* This function takes a file handle to a client, reads in the request,
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters:
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static void serve_client( int fd, Scheduler* sched, size_t http_size ) {
    static char *buffer;                        /* request buffer */
    char *req = NULL;                           /* ptr to req file */
    char *brk;                                  /* state used by strtok */
    char *tmp;                                  /* error checking ptr */
    FILE *fin;                                  /* input file handle */
    int len;                                    /* length of data read */

    if( !buffer ) {                             /* 1st time, alloc buffer */
        buffer = malloc( http_size );
        if( !buffer ) {                     /* error check */
            perror( "Error while allocating memory" );
            abort();
        }
    }

    memset( buffer, 0, http_size );
    if( read( fd, buffer, http_size ) <= 0 ) { /* read req from client */
        perror( "Error while reading request" );
        abort();
    }

    /* standard requests are of the form
     *   GET /foo/bar/qux.html HTTP/1.1
     * We want the second token (the file path).
     */
    tmp = strtok_r( buffer, " ", &brk );        /* parse request */
    if( tmp && !strcmp( "GET", tmp ) ) {
        req = strtok_r( NULL, " ", &brk );
    }

    if( !req ) {                                /* is req valid? */
        len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
        write( fd, buffer, len );           /* if not, send err */
        close( fd );
    } else {                                    /* if so, open file */
        req++;                              /* skip leading / */
        fin = fopen( req, "r" );            /* open file */
        if( !fin ) {                        /* check if successful */
            len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );
            write( fd, buffer, len );   /* if not, send err */
            close( fd );
        } else {                            /* if so, send file */

            // This will give us the size
            struct stat buf;
            fstat(fileno(fin), &buf);
            

            //create request control block
            RCB* rcb = (RCB*)malloc(sizeof(RCB));
            rcb->sequenceNum = g_sequenceCounter++;
            rcb->clientFD = fd;
            rcb->handle = fin;
            rcb->numBytesRemaining = (int)buf.st_size; // buf gives us the size remaining
            //For SJF quantum is the entire file, call function to add RCB to correct position in queue
            
            // instead of inserting to sched, insert to worker queue
            struct WorkerNode* workerNode = createWorkerNode(rcb);
            addWorkerToQueue(workerNode, &workerThreadData.workerQueue);
            //workerThreadData.workerQueue = workerQueue;
            
            if(sched->type ==1){
                rcb->quantum = (int)buf.st_size;
            }else{
                rcb->quantum = http_size;
            }
            
            /////////move parts of this to pthread
            
//            if(sched->type ==1){
//                rcb->quantum = (int)buf.st_size;
//                addRCBtoQueueForSJF(rcb, sched);
//            }
//                //For RR and MLFB, quantum is the size parameter, call function to add RCB to end of queue
//            else{
//                rcb->quantum = http_size;
//                //call the scheduler function to put this in the queue
//                addRCBtoQueue(rcb, sched);
//            }
            len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" ); /* send success code */
            write( fd, buffer, len );

        }
    }
    //free(buffer);

    //close( fd );                                     /* close client connectuin*/
}
// process request for Shortest Job First
void processRequestSJF(RCB* rcb, Scheduler* sched) {
    // send entire file to the client
    static char *buffer;                                  /* request buffer */
    buffer = malloc( rcb->quantum );
    if( !buffer ) {                     /* error check */
        perror( "Error while allocating memory" );
        abort();
    }
    long len;                                              /* length of data read */

    len = fread( buffer, 1, rcb->quantum, rcb->handle); /* read file chunk */

    //DEBUGGING, REMOVE
    printf("QUANTUM: %d LENGTH: %ld \n", rcb->quantum, len);

    if( len < 0 ) {                                 /* check for errors */
        perror( "Error while writing to client" );
    } else if( len > 0 ) {                            /* if none, send chunk */

        len = write(rcb->clientFD, buffer, len);
        // subtract from bytes remaining
        rcb->numBytesRemaining -= len;
        if( rcb->numBytesRemaining != 0 ) {                           /* check for errors */
            perror( "File was not transferred completely as expected" );
        }
    }                 /* the last chunk < 8192 */

    //printf("%s\n", buffer);

    // close the file and connection
    //need to free things
    fclose(rcb->handle);
    close(rcb->clientFD);
    free(rcb);
}

// process request for Round Robin
void processRequestRR(RCB* rcb, Scheduler* sched) {
    // send part of it to the client
    static char *buffer;                                  /* request buffer */
    //if( !buffer ) {                             /* 1st time, alloc buffer */
    buffer = malloc( MAX_HTTP_SIZE_8KB );
    if( !buffer ) {                     /* error check */
        perror( "Error while allocating memory" );
        abort();
    }
    //}
    long len;                                              /* length of data read */

    len = fread( buffer, 1, MAX_HTTP_SIZE_8KB, rcb->handle); /* read file chunk */

    //DEBUGGING, REMOVE
    printf("QUANTUM: %d LENGTH: %ld \n", rcb->quantum, len);

    if( len < 0 ) {                                 /* check for errors */
        perror( "Error while writing to client" );
    } else if( len > 0 ) {                            /* if none, send chunk */

        len = write(rcb->clientFD, buffer, len);
        // subtract from bytes remaining
        rcb->numBytesRemaining -= len;
        if( len < 1 ) {                           /* check for errors */
            perror( "Error while writing to client" );
        }
    }                 /* the last chunk < 8192 */

    //printf("%s\n", buffer);

    // if this was the end, close the file and connection,
    //otherwise add it to the end of the queue
    if (rcb->numBytesRemaining <= 0) {
        //need to free things


        fclose(rcb->handle);
        close(rcb->clientFD);
        free(rcb);
        
    } else {
        addRCBtoQueue(rcb, sched);
    }
}

// process request for Multilevel Queue with Feedback
void processRequestMLFB(RCB* rcb, Scheduler* nextLevelSchedule, size_t max_size, size_t next_size) {
    static char *buffer;                            /* request buffer */
    //reallocate buffer
    buffer = malloc( max_size );
    if( !buffer ) {         /* error check */
        perror( "Error while allocating memory" );
        abort();
    }
    long len;                                        /* length of data read */

    len = fread( buffer, 1, max_size, rcb->handle); /* read file chunk */

    //DEBUGGING, REMOVE
    printf("QUANTUM: %d LENGTH: %ld \n", rcb->quantum, len);

    if( len < 0 ) {                           /* check for errors */
        perror( "Error while writing to client" );
    } else if( len > 0 ) {                      /* if none, send chunk */

        len = write(rcb->clientFD, buffer, len);
        // subtract from bytes remaining
        rcb->numBytesRemaining -= len;
        if( len < 1 ) {               /* check for errors */
            perror( "Error while writing to client" );
        }
    }           /* the last chunk < 8192 */

    //printf("%s\n", buffer);

    // if this was the end, close the file and connection,
    //otherwise add it to the end of the queue
    if (rcb->numBytesRemaining <= 0) {
        //need to free things
        fclose(rcb->handle);
        close(rcb->clientFD);
        free(rcb);
    } else {
        if (nextLevelSchedule == NULL) {
            //scheduler needs to be initialized for next queue
            serve_client( rcb->clientFD, nextLevelSchedule, next_size);
        } else {
            //add to the next level priority queue
            addRCBtoQueue(rcb, nextLevelSchedule);
        }
    }
}
/*
 * Process requests and write their files to the output
 * Parameters:
 *              Each type of scheduler
 */
static void * ProcessRequests(void * args) {
    WorkerThreadData* myWorkerThreadData = (WorkerThreadData*)args;

    for(;; ) {                                  /* main loop */

        //if we see an rcb, wake up
        if (myWorkerThreadData->workerQueue != NULL && myWorkerThreadData->workerQueue->rcb != NULL){
         
            //pop now
            struct WorkerNode *wq = popFrontWorkerQueue(&myWorkerThreadData->workerQueue);
            
            
            if(myWorkerThreadData->sched->type ==1){
                addRCBtoQueueForSJF(wq->rcb, myWorkerThreadData->sched);
            }
                //For RR and MLFB, quantum is the size parameter, call function to add RCB to end of queue
            else{
                //workerThreadData.workerQueue->rcb->quantum = http_size;
                //call the scheduler function to put this in the queue
                addRCBtoQueue(wq->rcb, myWorkerThreadData->sched);
            }
            
            
            while (type_SJF && schedSJF.requestTable != NULL) {
                RCB* next = getNextRCB(&schedSJF);
                if (next != NULL) {
                    //for debuging only
                    printf( "Processing Shortest Job First Queue.\n" );
                    //process all requests in SJF
                    processRequestSJF(next, &schedSJF);
                }
            }
            
            while (type_MLFB && schedHIGH.requestTable != NULL) {
                RCB* next = getNextRCB(&schedHIGH);
                if (next != NULL) {
                    //for debug
                    printf( "Processing high priority queue\n" );
                    //only the mlfb uses this scheduler
                    //processRequestMLFB(next RCB, next scheduler to put file in, this que size, next que size)
                    processRequestMLFB(next, &schedMED, MAX_HTTP_SIZE_8KB, MAX_HTTP_SIZE_64KB);
                }
            }
            
            //check the medium priority queue, it should break if there is a new file in high priority
            while (type_MLFB && schedMED.requestTable != NULL && schedHIGH.requestTable == NULL) {
                RCB* next = getNextRCB(&schedMED);
                if (next != NULL) {
                    //for debug only
                    printf( "Processing medium priority queue.\n" );
                    processRequestMLFB(next, &schedRR, MAX_HTTP_SIZE_64KB, MAX_HTTP_SIZE_8KB);
                }
            }
            
            while ((type_MLFB || type_RR) && schedRR.requestTable != NULL && schedMED.requestTable == NULL
                   && schedHIGH.requestTable == NULL) {
                RCB* next = getNextRCB(&schedRR);
                if (next != NULL) {
                    //for debug only
                    printf( "Processing round robin queue.\n" );
                    //process all requests in round robin
                    processRequestRR(next, &schedRR);
                }
            }
        }

        
    }
}


/* This function is where the program starts running.
 *    The function first parses its command line parameters to determine port #
 *    Then, it initializes, the network and enters the main loop.
 *    The main loop waits for a client (1 or more to connect, and then processes
 *    all clients by calling the seve_client() function for each one.
 * Parameters:
 *             argc : number of command line parameters (including program name
 *             argv : array of pointers to command line parameters
 * Returns: an integer status code, 0 for success, something else for error.
 */
int main( int argc, char **argv ) {
    //workerThreadData = malloc(sizeof(workerThreadData));
    workerThreadData.workerQueue = workerQueue;
    int port = -1;                              /* server port # */
    /* client file descriptor */
    int fd;
    pthread_t tid;
    //init schedulers
    schedSJF.requestTable = NULL;
    schedSJF.type = SJF;
    //round robin
    schedRR.requestTable = NULL;
    schedRR.type = RR;
    //high priority
    schedHIGH.requestTable = NULL;
    schedHIGH.type = MLFB;
    //medium priority
    schedMED.requestTable = NULL;
    schedMED.type = MLFB;

    //positive integer denoting the number of worker threads to create
    int num_threads = -1;

    
    /* check for and process parameters
     * Takes 3 args
     */
    if( ( argc  > 4 ) || ( argc < 4 ) || ( sscanf( argv[1], "%d", &port ) < 0 )
       || ( sscanf( argv[3], "%d", &num_threads ) < 1 ) ) {
        printf( "usage: sms <port> <scheduler> <threads>\n" );
        return 0;
    }
    
    //find the scheduler type
    if (strcmp(argv[2], "RR") == 0 || strcmp(argv[2], "rr") == 0) {
        type_RR = 1;
    }
    else if (strcmp(argv[2], "SJF") == 0 || strcmp(argv[2], "sjf") == 0) {
        type_SJF = 1;
    }
    else if (strcmp(argv[2], "MLFB") == 0 || strcmp(argv[2], "mlfb") == 0) {
        type_MLFB = 1;
    }
    else {
        printf( "invalid scheduler\n" );
        return 0;
    }
    
    if (type_RR) {
        workerThreadData.sched = &schedRR;
    } else if (type_SJF) {
        workerThreadData.sched = &schedSJF;
    } else if (type_MLFB) {
        workerThreadData.sched = &schedHIGH;
    }
    
    

    network_init( port );                       /* init network module */

    //Initialize threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&tid, NULL, &ProcessRequests, &workerThreadData);
    }

    

    for(;; ) {                                  /* main loop */
        network_wait();                   /* wait for clients */

        //TODO: enqueue new thread here
        for (fd = network_open();
             fd >= 0; fd = network_open()) { /* get clients */                           /* process each client */
            // create scheduler here, RR for now
            if (type_RR) {
                
                //workerThreadData.sched = &schedRR;
                serve_client(fd, &schedRR, MAX_HTTP_SIZE_8KB);
            } else if (type_SJF) {
                //workerThreadData.sched = &schedSJF;
                serve_client(fd, &schedSJF, MAX_HTTP_SIZE_8KB);
            } else if (type_MLFB) {
                //workerThreadData.sched = &schedHIGH;
                //start mlfb in high schedule priority queue
                serve_client(fd, &schedHIGH, MAX_HTTP_SIZE_8KB);
            }
        }
    }
}