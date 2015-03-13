#include "elevDriver.h"
#include "costFunction.h"
#include <stdlib.h>
#include <pthread.h>

struct order{
	int from;
	int dest;
	int buttonType;
	int elevator
};
struct{
	struct order Queue[100];
	int inUse[100];
	int costOfQueue[100];
	int localPri[100];
	pthread_mutex_t rwLock;
}orderQueue;



void initPriorityQueue(){
	int i;
	for (i = 0; i < 100; i++){
		orderQueue.inUse[i] = 0;
		orderQueue.costOfQueue[i] = -1;
		orderQueue.localPri[i] = -1;

	}
	pthread_mutex_init(&(orderQueue.rwLock), NULL);
}

void addNewOrder(struct order newOrder){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int pos = 0;
	while(orderQueue.inUse[pos]){
		pos++
		if (pos == 100){
			fprintf("Error: orderQueue is full, order not received");
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return;
		}
	}
	orderQueue.Queue[pos] = newOrder;
	orderQueue.inUse[pos] = 1;
	orderQueue.costOfQueue[pos] = 0; //temp
	if (newOrder.buttonType == BUTTON_COMMAND){
		orderQueue.localPri[pos] = newOrder.elevator;
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
}

int getNewOrder(int elevator, int elevatorFloor){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int i;
	for (i = 0; i < 100; 1++){
		if (orderQueue.localPri[i] == elevator){
			//Prioritizes commands from the buttons inside the elevator
			orderQueue.inUse[i] = 0;
			orderQueue.costOfQueue[i] = -1;
			orderQueue.localPri[i] = -1;
			int destFloor = orderQueue.Queue.dest;
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return destFloor;
		}
	}
	int cost = lowestCost(elevatorFloor);

	pthread_mutex_unlock(&(orderQueue.rwLock));
	return 
}


int lowestCost(int curFloor){
	int i;
	for (i = 0; i < 100; i++){
		if ((orderQueue.localPri[i] == -1) && (orderQueue.inUse[i] == 1)){
			//orderQueue.costOfQueue[i] -= 1;
		}
	}
}