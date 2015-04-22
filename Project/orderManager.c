#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include "orderManager.h"
#include "network_modulev2.h"
#include "mainDriver.h"
#include "costFunction.h"
#include "elevDriver.h"

int localQueue[N_FLOORS];
sem_t timeoutSem;
int MASTER; //Forward declaration?

void* orderManager(void* args){
	//int masterStatus = *(int *) args);
	sem_init(&timeoutSem, 0, 0);
	initPriorityQueue();
	pthread_t sendMessages, receiveMessages, sortMessages_, masterTimeout_;
	pthread_create(&sendMessages, 0, &send_message, 0);
	pthread_create(&receiveMessages, 0, &listen_for_messages, 0);
	pthread_create(&sortMessages_, 0, &sortMessages, 0);
	pthread_create(&masterTimeout_, 0, &masterTimeout, 0);


}

int addNewOrder(struct order newOrder, int currentFloor, int nextFloor){
	if (MASTER == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		int pos = 0;
		struct order storeOrder;
		storeOrder.dest = newOrder.dest;
		storeOrder.buttonType = newOrder.buttonType;
		storeOrder.elevator = newOrder.elevator;
		
		while(orderQueue.inUse[pos]){
			pos++;
			if (pos == N_ORDERS){
				printf("Error: orderQueue is full, order not received\n");
				pthread_mutex_unlock(&(orderQueue.rwLock));
				return;
			}
		}
		if (storeOrder.buttonType != BUTTON_COMMAND){
			printf("Button lamp on: floor: %d, type: %d\n", storeOrder.dest, storeOrder.buttonType);
			setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
			BufferInfo newMsg;
			encodeMessage(newMsg, NULL, NULL, MSG_SET_LAMP, storeOrder.nextFloor, storeOrder.buttonType, 1);
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
		}
		orderQueue.Queue[pos] = storeOrder;
		orderQueue.inUse[pos] = 1;
		if (storeOrder.buttonType == BUTTON_COMMAND){
			orderQueue.localPri[pos] = storeOrder.elevator;
		}
		pthread_mutex_unlock(&(orderQueue.rwLock));

	}else{ //Send new order to the master
		BufferInfo newMsg;
		encodeMessage(newMsg, NULL, NULL, MSG_ADD_ORDER, newOrder.dest, newOrder.buttonType, 1);
		newMsg.currentFloor = currentFloor;
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor){
	int destFloor;
	int dir = nextFloor - currentFloor;
	if (nextFloor == -1) dir = 0;
	if (MASTER == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		destFloor = findLowestCost(orderQueue.localPri,orderQueue.inUse,orderQueue.Queue,currentFloor, nextFloor);
		pthread_mutex_unlock(&(orderQueue.rwLock));
	}else{
		int i, tmp;
		int destFloor = -1;
		int minCost = N_FLOORS * 2;
		for (i = 0; i < N_FLOORS; i++){
			if (localQueue[i] == 1){
				tmp = findCost(i, currentFloor, nextFloor);
				if (tmp < minCost){
					minCost = tmp;
					destFloor = i;
				}
			}
		} 
		localQueue[destFloor] = 0;
	}
	return destFloor;
}

void* sortMessages(void *args){
	BufferInfo bufOrder;
	int myIP = getLocalIP();
	int broadcast = getBroadcastIP();
	while(1){
		wait_for_content();
		dequeue(receiveQueue, &bufOrder);
		int myState = bufOrder.myState;
		int dstAddr = inet_addr(bufOrder.dstAddr);
		int srcAddr = inet_addr(bufOrder.srcAddr);
		if ((myIP == dstAddr) || (broadcast == dstAddr)){
			if (myState == MSG_DO_ORDER){
				localQueue[bufOrder.nextFloor] = 1;
			}
			if (myState == MSG_SET_LAMP){
				setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
			}
			if (myState == MSG_CONNECT_SEND){
				addElevatorAddr(bufOrder.srcAddr);
				BufferInfo newMsg;
				encodeMessage(newMsg, NULL, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, MASTER, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(newMsg));
			}

			if (MASTER == 1){
				if (myState = MSG_ADD_ORDER){
					struct order newOrder;
					newOrder.dest = bufOrder.nextFloor;
					newOrder.buttonType = bufOrder.buttonType;
					newOrder.elevator = inet_aton(bufOrder.srcAddr);
					addNewOrder(newOrder, bufOrder.currentFloor,bufOrder.nextFloor);
					if (bufOrder.buttonType == BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(newMsg, NULL, bufOrder.srcAddr, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
						enqueue(sendQueue, &newMsg, sizeof(newMsg));
					}
				}
			}else{
				if (myState == MSG_IM_ALIVE){
					sem_post(&timeoutSem);
				}
			}
		}
	}
}

void* masterTimeout(void *args){
	struct timespec ts, rem;
	
	if (MASTER == 0){
		ts.tv_sec = 5;
		ts.tv_nsec = 0;
		int test;
		while(1){
			test = sem_timedwait(&timeoutSem, &ts);
			if (test == -1){
				printf("Master timeout\n");
				//Someone becomes master
				return;
			}
		}
	}else{
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		BufferInfo = newMsg;
		encodeMessage(newMsg, NULL, NULL, MSG_IM_ALIVE, 1, -1, -1);
		while(1){
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
			nanosleep(ts,rem);

		}
	}
}

void deleteOrder(int floor, buttonType button){
	if (MASTER == 1){
		int i;
		for (i = 0; i < N_ORDERS; i++){
			if(orderQueue.Queue[i].dest == floor && orderQueue.Queue[i].buttonType == button){
				orderQueue.inUse[i] = 0;
				orderQueue.localPri[i] = -1;
			}
		}
	}
}