#include "fifoqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "network_modulev2.h"

fifoqueue_t* new_fifoqueue(void){
    fifoqueue_t* q = malloc(sizeof(fifoqueue_t));
    q->front = NULL;
    
    pthread_mutex_init(&q->mtx, NULL);
    sem_init(&q->sem, 0, 0);
    
    return q;
}

void enqueue(fifoqueue_t* q, BufferInfo* data, size_t size){
    pthread_mutex_lock(&q->mtx);
    //size_t size = sizeof(char) * length;
    fifonode_t* newnode = malloc(sizeof(fifonode_t));
    //int type = 1;
    //newnode->type = type;
    newnode->data = malloc(size);
    newnode->size = size;
    newnode->next = NULL;
    
    memcpy(newnode->data, data, size);

    if(q->front == NULL){
        q->front = newnode;
    } else {
        fifonode_t* n = q->front;
        while(n->next != NULL){
            n = n->next;
        }
        n->next = newnode;
    }
    sem_post(&q->sem);
    pthread_mutex_unlock(&q->mtx);
}

void dequeue(fifoqueue_t* q, BufferInfo* recv){
    pthread_mutex_lock(&q->mtx);

    if(q->front){
        memcpy(recv, q->front->data, q->front->size);

        fifonode_t* del = q->front;
        
        q->front = q->front->next;
        
        free(del->data);
        free(del);
    }
    
    pthread_mutex_unlock(&q->mtx);
}

void wait_for_content(fifoqueue_t* q){
    sem_wait(&q->sem);
}

int trywait_for_content(fifoqueue_t* q){
	return sem_trywait(&q->sem);
}

void delete_fifoqueue(fifoqueue_t** q){
    
    while((*q)->front != NULL){
        fifonode_t* del = (*q)->front;
        (*q)->front = (*q)->front->next;
        
        free(del->data);
        free(del);
    }
    
    pthread_mutex_destroy(&(*q)->mtx);
    
    free(*q);
}
