#include "elevDriver.h"
#include "costFunction.h"
#include <stdlib.h>
#include <pthread.h>

void initPriorityQueue(){
	int i;
	for (i = 0; i < 100; i++){
		orderQueue.inUse[i] = 0;
		orderQueue.localPri[i] = -1;

	}
	pthread_mutex_init(&(orderQueue.rwLock), NULL);
}

void addNewOrder(struct order newOrder){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int pos = 0;
	while(orderQueue.inUse[pos]){
		pos++;
		if (pos == 100){
			printf("Error: orderQueue is full, order not received");
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return;
		}
	}
	orderQueue.Queue[pos] = newOrder;
	orderQueue.inUse[pos] = 1;
	if (newOrder.buttonType == BUTTON_COMMAND){
		orderQueue.localPri[pos] = newOrder.elevator;
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
}

int getNewOrder(int currentFloor){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int i, destFloor;
	for (i = 0; i < 100; i++){
		if (orderQueue.Queue[i].elevator == 1){
			//Prioritizes commands from the buttons inside the elevator
			orderQueue.inUse[i] = 0;
			orderQueue.localPri[i] = -1;
			destFloor = orderQueue.Queue[i].dest;
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return destFloor;
		}
	}
	int cost = findLowestCost(orderQueue.localPri,orderQueue.inUse,orderQueue.Queue,currentFloor);
	for (i = 0; i < 100; i++){
		if ((findCost(orderQueue.Queue[i],currentFloor)) == cost){
			orderQueue.inUse[i] = 0;
			orderQueue.localPri[i] = -1;
			destFloor = orderQueue.Queue[i].dest;
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return destFloor;
		}
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
	return -1;
}


int findLowestCost(int priority[100] ,int inUse[100], struct order queue[100], int currentFloor){
	int i, minPos;
	int min = 4;
	for (i = 0; i < 100; i++){
		if ((priority[i] == -1) && (inUse[i] == 1)){
			//orderQueue.costOfQueue[i] -= 1;
			if(findCost(queue[i],currentFloor) < min){
				min = findCost(queue[i],currentFloor);
				minPos = i;
			}
		}
	}
	return min;
}

int findCost(struct order newOrder,int currentFloor){
	int cost = newOrder.dest - currentFloor;
	if (cost < 0){
		return (-1)*cost;
	} 
	return cost;
}