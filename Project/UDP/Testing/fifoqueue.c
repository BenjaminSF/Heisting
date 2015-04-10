

#include "fifoqueue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>




fifoqueue_t* new_fifoqueue(void){
    fifoqueue_t* q = malloc(sizeof(fifoqueue_t));
    q->front = NULL;
    
    pthread_mutex_init(&q->mtx, NULL);
    sem_init(&q->sem, 0, 0);
    
    return q;
}



void enqueue(fifoqueue_t* q, char* data, int length){
    pthread_mutex_lock(&q->mtx);
    size_t size = sizeof(char) * length;
    fifonode_t* newnode = malloc(sizeof(fifonode_t));
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



void dequeue(fifoqueue_t* q, char* recv){
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

/*void pop_front(fifoqueue_t* q){
    pthread_mutex_lock(&q->mtx);

    if(q->front){
        fifonode_t* del = q->front;
        
        q->front = q->front->next;
        
        free(del->data);
        free(del);
    }
    
    pthread_mutex_unlock(&q->mtx);
}*/

void wait_for_content(fifoqueue_t* q){
    sem_wait(&q->sem);
}

/*int front_type(fifoqueue_t* q){
    pthread_mutex_lock(&q->mtx);
    int res;
    if(q->front == NULL){
        res = 0;
    } else {
        res = q->front->type;
    }
    pthread_mutex_unlock(&q->mtx);
    return res;
}*/


/*
void print_fifonode_t(fifonode_t* n){
    printf("fifonode_t(%d, %p, %lu, %p)\n", n->type, n->data, n->size, n->next);
}
*/

/*
void print_fifoqueue_t(fifoqueue_t* q){
    pthread_mutex_lock(&q->mtx);
    for(fifonode_t* n = q->front; n != NULL; n = n->next){
        print_fifonode_t(n);
    }
    pthread_mutex_unlock(&q->mtx);
}
*/


void delete_fifoqueue(fifoqueue_t** q){
    
    while(front_type(*q) != 0){
        fifonode_t* del = (*q)->front;
        (*q)->front = (*q)->front->next;
        
        free(del->data);
        free(del);
    }
    
    pthread_mutex_destroy(&(*q)->mtx);
    
    free(*q);
}
