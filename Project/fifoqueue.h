#pragma once

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "network_modulev2.h"

/** An untyped mutex-protected first-in first-out queue.
    Recommended usage:
        fifoqueue_scoped* q = new_fifoqueue();
        A a; // instance of type A, with type id "typeA"

        while(1){
            wait_for_content(q);
            switch(front_type(q)){
                case typeA:
                    dequeue(q, &a);
                    // handle type A here
                default:
                    // includes case 0 (queue empty)
            }
        }

    call enqueue() in other threads or when handling other types
*/


/** The base type. Prefer declaring queues with fifoqueue_scoped.
*   Initialize a new queue with new_fifoqueue();
*/
typedef struct fifoqueue_t fifoqueue_t;


/// fifoqueue_scoped is a wrapper around an fifoqueue_t, such that it is automatically freed at scope exit.
#define fifoqueue_scoped __attribute__((cleanup(delete_fifoqueue))) fifoqueue_t


/// Returns a fresh fifoqueue_t from the heap.
fifoqueue_t* new_fifoqueue(void);


/** Copies data of _size size into the back of the queue.
*   The caller defines a type id, that can be retrieved with frontType()
*       Type id 0 is reserved (see frontType())
*   enqueue does not do a deep copy of data
*/
void enqueue(fifoqueue_t* q, BufferInfo* message, size_t size);


/** Copies the front element of the queue into recv
*   Use frontType() to get the type of the front element
*/
void dequeue(fifoqueue_t* q, BufferInfo* recv);

/** Similar to dequeue(), but it does not copy the element
*/
//void pop_front(fifoqueue_t* q);

/** Blocking call: Waits until there is content in the queue
*/
void wait_for_content(fifoqueue_t* q);

/** Non-blocking call: returns 0 if there is content available (and decrements the semaphore), otherwise it returns -1
*/
int trywait_for_content(fifoqueue_t* q);

/** Returns the type id of the front element, as set in enqueue()
    Returns 0 if the queue is empty
*/
//int front_type(fifoqueue_t* q);


//void print_fifoqueue_t(fifoqueue_t* q);


/** Frees all data in a fifoqueue_t
*   Called by fifoqueue_scoped on scope exit
*/
void delete_fifoqueue(fifoqueue_t** q);



// INTERNAL
typedef struct fifonode_t fifonode_t;
struct fifonode_t {
    //int                 type;
    char*               data;
    
    size_t              size;
    fifonode_t*         next;
};


struct fifoqueue_t {
    fifonode_t*         front;
    pthread_mutex_t     mtx;
    sem_t               sem;
};

