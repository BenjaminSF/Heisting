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
	struct order Queue[N_ORDERS];
	int inUse[N_ORDERS];
	int localPri[N_ORDERS];
	int enRoute[N_ORDERS];
	pthread_mutex_t rwLock;
}orderQueue;


void initPriorityQueue();
int findCost(int costFloor,int currentFloor, int nextFloor,int buttonType);
//void deleteOrder(int floor, buttonType button);

#endif