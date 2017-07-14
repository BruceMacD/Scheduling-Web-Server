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

#include "network.h"
#include "request-table.h"

#define MAX_HTTP_SIZE_8KB 8192
#define MAX_HTTP_SIZE_64KB 65536                 /* size of buffer to allocate */

// global sequence counter
int g_sequenceCounter = 1;

/* This function takes a file handle to a client, reads in the request,
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters:
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static void serve_client( int fd, Scheduler* sched, int http_size ) {
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
                        rcb->quantum = 8192; // this is based on which scheduler, this is correct for RR

                        //call the scheduler function to put this in the queue
                        addRCBtoQueue(rcb, sched);

                        len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" ); /* send success code */
                        write( fd, buffer, len );

                }
        }
        //free(buffer);

//  close( fd );                                     /* close client connectuin*/
}


// process request for Round Robin
void processRequestRR(RCB* rcb, Scheduler* sched) {
        // send part of it to the client
        static char *buffer;                                  /* request buffer */
        if( !buffer ) {                             /* 1st time, alloc buffer */
                buffer = malloc( MAX_HTTP_SIZE_8KB );
                if( !buffer ) {                     /* error check */
                        perror( "Error while allocating memory" );
                        abort();
                }
        }
        long len;                                              /* length of data read */

        len = fread( buffer, 1, MAX_HTTP_SIZE_8KB, rcb->handle); /* read file chunk */
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
                free(rcb);
                fclose(rcb->handle);
                close(rcb->clientFD);
        } else {
                addRCBtoQueue(rcb, sched);
        }
}

// process request for Multilevel Queue with Feedback
void processRequestMLFB(RCB* rcb, Scheduler* sched) {
        //TODO
        //init three queues
        //8kb high priority
        //64kb med priority
        //round robin low priority
        // send part of it to the client
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
        int port = -1;                              /* server port # */
        int fd;                                     /* client file descriptor */
        //TODO: for some reason removing these char arrays causes a seg fault, any ideas? - Bruce
        //shortest job first
        char type_sjf[6] = "SJF";
        //multilevel queue with feedback
        char type_mlq[6] = "MLQ";
        int type_SJF = 0;
        int type_RR = 0;
        int type_MLFB = 0;
        /* check for and process parameters
         */
        // TODO - read scheduler to choose scheduler type
        if( ( argc < 3 ) || ( sscanf( argv[1], "%d", &port ) < 1 ) ) {
                printf( "usage: sms <port> <scheduler>\n" );
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

        network_init( port );                       /* init network module */

        // The schedulers
        Scheduler schedRR;
        schedRR.requestTable = NULL;
        Scheduler schedHIGH;
        schedHIGH.requestTable = NULL;
        Scheduler schedLOW;
        schedLOW.requestTable = NULL;

        //TODO: I think this creates different queues for each client, which I don't believe we want to do - Bruce
        for(;; ) {                                  /* main loop */
                network_wait();                   /* wait for clients */

                for( fd = network_open(); fd >= 0; fd = network_open() ) { /* get clients */                           /* process each client */
                        // create scheduler here, RR for now
                        if (type_RR) {
                                serve_client( fd, &schedRR, MAX_HTTP_SIZE_8KB );
                        }
                        else if (type_SJF) {
                                //TODO: SJF
                        }
                        else if (type_MLFB) {
                                //start mlfb in high schedule priority queue
                                //serve_client( fd, &schedHIGH, MAX_HTTP_SIZE_8KB );
                        }
                        else {
                                printf( "invalid scheduler\n" );
                                return 0;
                        }
                }
                // the next request
                //TODO: This needs to be able to run at the same time as serving clients or it isn't an effective queue
                //TODO: Should be moved to a separate function for the multithreading portion
                while (type_MLFB && schedHIGH.requestTable != NULL) {
                        RCB* next = getNextRCB(&schedHIGH);
                        if (next != NULL) {
                                //for debug
                                printf( "Processing high priority queue\n" );
                                //only the mlfb uses this scheduler
                                //processRequestMLFB(next, &schedHIGH, &schedLOW, &schedRR);
                        }
                }

                while (type_MLFB && schedLOW.requestTable != NULL) {
                        RCB* next = getNextRCB(&schedLOW);
                        if (next != NULL) {
                                //for debug only
                                printf( "Processing low priority queue.\n" );
                                //TODO
                        }
                }

                while ((type_MLFB || type_RR) && schedRR.requestTable != NULL) {
                        RCB* next = getNextRCB(&schedRR);
                        if (next != NULL) {
                                //process all requests in round robin
                                processRequestRR(next, &schedRR);
                        }
                }
        }
}
