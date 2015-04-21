#ifndef _costFunction_
#define _costFunction_

#include <stdio.h>
#include <pthread.h>
#define N_ORDERS 100

struct order{
	int dest;
	int buttonType;
	int elevator;
};
struct{
	struct order Queue[100];
	int inUse[100];
	int localPri[100];
	pthread_mutex_t rwLock;
}orderQueue;


void initPriorityQueue();
//void deleteOrder(int floor, buttonType button);

#endif