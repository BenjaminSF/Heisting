#ifndef _fifoqueue_
#define _fifoqueue_

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

struct BufferInfo;
typedef struct fifoqueue_t fifoqueue_t;

fifoqueue_t* newFifoqueue(void);
void enqueue(fifoqueue_t* q, struct BufferInfo* message, size_t size);
void dequeue(fifoqueue_t* q, struct BufferInfo* recv);
void waitForContent(fifoqueue_t* q);
void deleteFifoqueue(fifoqueue_t** q);

typedef struct fifonode_t fifonode_t;
struct fifonode_t {
    char*               data;
    
    size_t              size;
    fifonode_t*         next;
};

struct fifoqueue_t {
    fifonode_t*         front;
    pthread_mutex_t     mtx;
    sem_t               sem;
};

#endif