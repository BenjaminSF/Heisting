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
struct {
	int active[N_ELEVATORS];
	int floor[N_ELEVATORS];
	int direction[N_ELEVATORS];
} elevStates;
sem_t timeoutSem;
int bestProposal;
int MASTER; //Forward declaration?

void* orderManager(void* args){
	//int masterStatus = *(int *) args);
	//struct timespec startAnarchy;
	printf("Master: %d\n", MASTER);
	struct timespec sleep = {.tv_sec = 2, .tv_nsec = 0};
	struct timespec rem;
	bestProposal = getLocalIP();
	sem_init(&timeoutSem, 0, 0);
	//initPriorityQueue();
	pthread_t sendMessages, receiveMessages, sortMessages_, masterTimeout_;
	pthread_create(&sendMessages, 0, &send_message, 0);
	pthread_create(&receiveMessages, 0, &listen_for_messages, 0);
	pthread_create(&sortMessages_, 0, &sortMessages, 0);
	while(1){
		pthread_create(&masterTimeout_, 0, &masterTimeout, 0);
		pthread_join(masterTimeout_, 0);
		nanosleep(&sleep, &rem);
		if (bestProposal == getLocalIP()){
			MASTER = 1;
		}else{
			MASTER = 0;
		}
		setMasterIP(bestProposal);
		bestProposal = getLocalIP();
	}
	pthread_join(sendMessages, NULL);
	pthread_join(receiveMessages, NULL);
	pthread_join(sortMessages_, NULL);

}

int addNewOrder(struct order newOrder, int currentFloor, int nextFloor){
	printf("Enter addNewOrder\n");
	if (MASTER == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		//printf("Mutex owner: addNewOrder\n");
		int pos = 0;
		struct order storeOrder;
		storeOrder.dest = newOrder.dest;
		storeOrder.buttonType = newOrder.buttonType;
		storeOrder.elevator = newOrder.elevator;
		
		while(orderQueue.inUse[pos]){
			if(ordercmp(&(orderQueue.Queue[pos]), &storeOrder)){
				pthread_mutex_unlock(&(orderQueue.rwLock));
				return -1;
			} 
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
			encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, storeOrder.dest, storeOrder.buttonType, 1);
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
		}
		printf("Add order: dest: %d, button: %d, elev: %d\n", storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
		orderQueue.Queue[pos] = storeOrder;
		orderQueue.inUse[pos] = 1;
		if (storeOrder.buttonType == BUTTON_COMMAND){
			orderQueue.localPri[pos] = storeOrder.elevator;
			setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
		}
		//printf("Mutex released: addNewOrder\n");
		pthread_mutex_unlock(&(orderQueue.rwLock));

	}else{ //Send new order to the master
		printf("Dette er en slave\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_ADD_ORDER, newOrder.dest, newOrder.buttonType, 1);
		newMsg.currentFloor = currentFloor;
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor){
	//printf("Enter getNewOrder: currentFloor: %d, nextFloor: %d\n", currentFloor, nextFloor);
	int destFloor;
	int dir = nextFloor - currentFloor;
	if (nextFloor == -1) dir = 0;
	if (MASTER == 1){
		//printf("Tying to get mutex\n");
		pthread_mutex_lock(&(orderQueue.rwLock));
		//printf("Mutex owner: getNewORder\n");
		destFloor = findLowestCost(orderQueue.localPri,orderQueue.inUse,orderQueue.Queue,currentFloor, nextFloor);
		//printf("Mutex released: getNewORder\n");
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
		//localQueue[destFloor] = 0;
	}
	//printf("currentFloor: %d, nextFloor: %d, destFloor: %d\n", currentFloor, nextFloor, destFloor);

	return destFloor;
}

void distributeOrders(){ //Master only
	printf("Enter distributeOrders\n");
	int addrsCount, i, j, tmpAddr;
	//while(1){
		addrsCount = getAddrsCount();
		pthread_mutex_lock(&(orderQueue.rwLock));
		//printf("Mutex owner: distributeOrders\n");
		for (j = 0; j < N_ORDERS; j++){
			for (i = 0; i < addrsCount; i++){
				tmpAddr = addrsList(i);
				if (orderQueue.inUse[j] && (orderQueue.localPri[j] == tmpAddr)){
					
				}
			}
		}
		pthread_mutex_unlock(&(orderQueue.rwLock));
	//}
}

void* sortMessages(void *args){
	printf("Enter sortMessages\n");
	BufferInfo bufOrder;
	int myIP = getLocalIP();
	int broadcast = getBroadcastIP();
	while(1){
		//printf("Wait for content...\n");
		wait_for_content(receiveQueue);
		//printf("Message received, should never ever ever happen but sometimes it should\n");
		dequeue(receiveQueue, &bufOrder);
		int myState = bufOrder.myState;
		int dstAddr = bufOrder.dstAddr;
		int srcAddr = bufOrder.srcAddr;
		if ((myIP == dstAddr) || (broadcast == dstAddr)){
			if (myState == MSG_DO_ORDER){
				printf("Receive: DO_ORDER%d\n", bufOrder.nextFloor);
				localQueue[bufOrder.nextFloor] = 1;
			}
			if (myState == MSG_SET_LAMP){
				printf("Receive: MSG_SET_LAMP\n");
				setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
			}
			if (myState == MSG_CONNECT_SEND){
				printf("Receive: MSG_CONNECT_SEND\n");
				addElevatorAddr(bufOrder.srcAddr);
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, MASTER, -1, -1);
				enqueue(sendQueue, &newMsg, BUFFER_SIZE);
			}
			if (myState == MSG_MASTER_REQUEST){
				printf("Receive: MSG_MASTER_REQUEST\n");
				int candidate = 1;
				if (srcAddr > myIP){
					candidate = 0;
				}
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_MASTER_PROPOSAL, candidate, -1, -1);
				enqueue(sendQueue, &newMsg, BUFFER_SIZE);
			}
			if (myState == MSG_MASTER_PROPOSAL){
				printf("Receive: MSG_MASTER_PROPOSAL\n");
				if (srcAddr > bestProposal){
					bestProposal = srcAddr;
				}
			}

			if (MASTER == 1){
				if (myState == MSG_ADD_ORDER){
					printf("Receive: MSG_ADD_ORDER\n");
					struct order newOrder;
					newOrder.dest = bufOrder.nextFloor;
					newOrder.buttonType = bufOrder.buttonType;
					newOrder.elevator = srcAddr;
					addNewOrder(newOrder, bufOrder.currentFloor,bufOrder.nextFloor);
					if (bufOrder.buttonType == BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
						enqueue(sendQueue, &newMsg, BUFFER_SIZE);
					}
				}
				if(myState == MSG_DELETE_ORDER){
					printf("Receive: MSG_DELETE_ORDER\n");
					deleteOrder(bufOrder.currentFloor,bufOrder.buttonType,srcAddr);
					if (bufOrder.buttonType != BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.currentFloor, bufOrder.buttonType, 0);
						enqueue(sendQueue, &newMsg, BUFFER_SIZE);
					}
				}
			}else{
				if (myState == MSG_IM_ALIVE){
					printf("Receive: MSG_IM_ALIVE\n");
					sem_post(&timeoutSem);
				}
			}
		}
	}
}

void* masterTimeout(void *args){
	printf("Enter timeout\n");
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	struct timespec ts, rem;
	if (MASTER == 0){
		printf("Timer: slave\n");
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec = ts.tv_sec + 15;
		int test;
		while(1){
			test = sem_timedwait(&timeoutSem, &ts);
			if (test == -1){
				printf("Master timeout\n");
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_MASTER_REQUEST, -1, -1, -1);
				enqueue(sendQueue, &newMsg, BUFFER_SIZE);
				return;
			}
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec = ts.tv_sec + 5;
		}
	}else{
		printf("Timer: master\n");
		ts.tv_sec = 5;
		ts.tv_nsec = 0;
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_IM_ALIVE, 1, -1, -1);
		while(1){
			enqueue(sendQueue, &newMsg, BUFFER_SIZE);
			nanosleep(&ts, &rem);

		}
	}
}

void deleteOrder(int floor, buttonType button, int elevator){
	printf("Enter deleteOrder: floor: %d, button: %d, elev: %d\n", floor, button, elevator);
	if (MASTER == 1){
		int i;
		for (i = 0; i < N_ORDERS; i++){
			if (orderQueue.inUse[i] == 1){
				if (orderQueue.Queue[i].dest == floor && orderQueue.Queue[i].buttonType == button && orderQueue.Queue[i].elevator == elevator){
					orderQueue.inUse[i] = 0;
					orderQueue.localPri[i] = -1;
					printf("Deleting order!\n");
				}
			}
		}
	}else{
		BufferInfo msg;
		encodeMessage(&msg, 0, 0, MSG_DELETE_ORDER, floor, button, 1);
		enqueue(sendQueue, &msg, BUFFER_SIZE);
	}
	setButtonLamp(floor, button, 0);
	localQueue[floor] = 0;
}

int ordercmp(struct order *A, struct order *B){
	//printf("Enter ordercmp\n");
	int x = 1;
	if (A->dest != B->dest) x = 0;
	if (A->buttonType != B->buttonType) x = 0;
	if (A->elevator != B->elevator) x = 0;
	return x;
}