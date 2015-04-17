#include "elevDriver.h"
#include "costFunction.h"
#include <stdlib.h>
#include <pthread.h>
#define N_ORDERS 100
void initPriorityQueue(){
	int i;
	for (i = 0; i < N_ORDERS; i++){
		orderQueue.inUse[i] = 0;
		orderQueue.localPri[i] = -1;
		orderQueue.Queue[i].dest = 100;

	}
	pthread_mutex_init(&(orderQueue.rwLock), NULL);
}

void addNewOrder(struct order newOrder){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int pos = 0;
	struct order storeOrder;
	storeOrder.dest = newOrder.dest;
	storeOrder.buttonType = newOrder.buttonType;
	storeOrder.elevator = newOrder.elevator;
	printf("Button lamp on: floor: %d, type: %d\n", storeOrder.dest, storeOrder.buttonType);
	setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
	while(orderQueue.inUse[pos]){
		pos++;
		if (pos == N_ORDERS){
			printf("Error: orderQueue is full, order not received\n");
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return;
		}
	}
	orderQueue.Queue[pos] = storeOrder;
	orderQueue.inUse[pos] = 1;
	if (storeOrder.buttonType == BUTTON_COMMAND){
		orderQueue.localPri[pos] = storeOrder.elevator;
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
}

int getNewOrder(int currentFloor){
	pthread_mutex_lock(&(orderQueue.rwLock));
	int i, destFloor;
	for (i = 0; i < N_ORDERS; i++){
		if ((orderQueue.inUse[i]) && (orderQueue.Queue[i].buttonType == BUTTON_COMMAND)){
			//Prioritizes commands from the buttons inside the elevator
			orderQueue.inUse[i] = 0;
			orderQueue.localPri[i] = -1;
			orderQueue.Queue[i].elevator = 0;
			destFloor = orderQueue.Queue[i].dest;
			printf("Button lamp off1: floor: %d, type: %d\n", orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType);
			setButtonLamp(orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, 0);
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return destFloor;
		}
	}
	destFloor = findLowestCost(orderQueue.localPri,orderQueue.inUse,orderQueue.Queue,currentFloor);
	//setButtonLamp(orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, 0);

	pthread_mutex_unlock(&(orderQueue.rwLock));
	return destFloor;
}


int findLowestCost(int priority[100] ,int inUse[100], struct order queue[100], int currentFloor){
	int i, minPos,cost, backlog;
	int queuePos = 0;
	int min = 4;
	backlog = 0;
	for (i = 0; i < N_ORDERS; i++){
		if ((priority[i] == -1) && (inUse[i] == 1)){
			cost = findCost(queue[i],currentFloor);
			if(cost < min){
				min = cost;
				minPos = queue[i].dest;
				queuePos = i;
			}
		}
		if (inUse[i] == 1){
			backlog++;
		}
	}
	if (inUse[queuePos] == 1){
		printf("Button lamp off2: floor: %d, type: %d\n", queue[queuePos].dest, queue[queuePos].buttonType);
		setButtonLamp(queue[queuePos].dest, queue[queuePos].buttonType, 0);
	}
	inUse[queuePos] = 0;
	priority[queuePos] = -1;
	queue[queuePos].elevator = 0;
	
	printf("min cost: %d and pos: %d, backlog: %d\n",min,minPos, backlog);
	if (min == 4){
		return -1;
	}else{
		return minPos;
	}
}

int findCost(struct order newOrder,int currentFloor){
	int cost = newOrder.dest - currentFloor;
	if (cost < 0){
		return (-1)*cost;
	} 
	return cost;
}