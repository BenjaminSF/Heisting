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

void initPriorityQueue();
int findCost(int costFloor,int currentFloor, int nextFloor,int buttonType);
//void deleteOrder(int floor, buttonType button);

#endif