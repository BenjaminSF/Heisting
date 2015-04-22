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
		printf("Kanskje kommer man hit\n");
		pthread_create(&masterTimeout_, 0, &masterTimeout, 0);
		pthread_join(masterTimeout_, 0);
		printf("Burde ikke komme hit\n");
		nanosleep(&sleep, &rem);
		if (bestProposal == getLocalIP()){
			MASTER = 1;
		}else{
			MASTER = 0;
		}
		setMasterIP(bestProposal);
		bestProposal = getLocalIP();
	}
	printf("Dette er for langt\n");
	pthread_join(sendMessages, NULL);
	pthread_join(receiveMessages, NULL);
	pthread_join(sortMessages_, NULL);

}

int addNewOrder(struct order newOrder, int currentFloor, int nextFloor){
	printf("Enter addNewOrder\n");
	if (MASTER == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		int pos = 0;
		struct order storeOrder;
		storeOrder.dest = newOrder.dest;
		storeOrder.buttonType = newOrder.buttonType;
		storeOrder.elevator = newOrder.elevator;
		
		while(orderQueue.inUse[pos]){
			if(ordercmp(&(orderQueue.Queue[pos]), &storeOrder)) return -1; //Ignore duplicate orders
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
			encodeMessage(&newMsg, NULL, NULL, MSG_SET_LAMP, storeOrder.dest, storeOrder.buttonType, 1);
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
		}
		printf("Add order: dest: %d, button: %d, elev: %d\n", storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
		orderQueue.Queue[pos] = storeOrder;
		orderQueue.inUse[pos] = 1;
		if (storeOrder.buttonType == BUTTON_COMMAND){
			orderQueue.localPri[pos] = storeOrder.elevator;
		}
		pthread_mutex_unlock(&(orderQueue.rwLock));

	}else{ //Send new order to the master
		printf("Dette er en slave\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, NULL, NULL, MSG_ADD_ORDER, newOrder.dest, newOrder.buttonType, 1);
		newMsg.currentFloor = currentFloor;
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor){
	//printf("Enter getNewOrder\n");
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
		printf("Wait for content...\n");
		wait_for_content(receiveQueue);
		printf("Message received, should never ever ever happen but sometimes it should\n");
		dequeue(receiveQueue, &bufOrder);
		int myState = bufOrder.myState;
		int dstAddr = inet_addr(&bufOrder.dstAddr);
		int srcAddr = inet_addr(&bufOrder.srcAddr);
		if ((myIP == dstAddr) || (broadcast == dstAddr)){
			if (myState == MSG_DO_ORDER){
				printf("DO_ORDER: %d\n", bufOrder.nextFloor);
				localQueue[bufOrder.nextFloor] = 1;
			}
			if (myState == MSG_SET_LAMP){
				setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
			}
			if (myState == MSG_CONNECT_SEND){
				addElevatorAddr(bufOrder.srcAddr);
				BufferInfo newMsg;
				encodeMessage(&newMsg, NULL, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, MASTER, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(newMsg));
			}
			if (myState == MSG_MASTER_REQUEST){
				int candidate = 1;
				if (srcAddr > myIP){
					candidate = 0;
				}
				BufferInfo newMsg;
				encodeMessage(&newMsg, NULL, NULL, MSG_MASTER_PROPOSAL, candidate, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(newMsg));
			}
			if (myState == MSG_MASTER_PROPOSAL){
				if (srcAddr > bestProposal){
					bestProposal = srcAddr;
				}
			}

			if (MASTER == 1){
				if (myState == MSG_ADD_ORDER){
					struct order newOrder;
					newOrder.dest = bufOrder.nextFloor;
					newOrder.buttonType = bufOrder.buttonType;
					newOrder.elevator = srcAddr;
					addNewOrder(newOrder, bufOrder.currentFloor,bufOrder.nextFloor);
					if (bufOrder.buttonType == BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(&newMsg, NULL, bufOrder.srcAddr, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
						enqueue(sendQueue, &newMsg, sizeof(newMsg));
					}
				}
				if(myState == MSG_DELETE_ORDER){
					deleteOrder(bufOrder.currentFloor,bufOrder.buttonType,srcAddr);
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
	printf("Enter timeout\n");
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	struct timespec ts, rem;
	if (MASTER == 0){
		printf("Timer: slave\n");
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec = ts.tv_sec + 5;
		int test;
		while(1){
			test = sem_timedwait(&timeoutSem, &ts);
			if (test == -1){
				printf("Master timeout\n");
				BufferInfo newMsg;
				encodeMessage(&newMsg, NULL, NULL, MSG_MASTER_REQUEST, -1, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(newMsg));
				return;
			}
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec = ts.tv_sec + 5;
		}
	}else{
		printf("Timer: master\n");
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		BufferInfo newMsg;
		encodeMessage(&newMsg, NULL, NULL, MSG_IM_ALIVE, 1, -1, -1);
		while(1){
			printf("Timer: Enqueue og sleep\n");
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
			nanosleep(&ts, &rem);

		}
	}
}

void deleteOrder(int floor, buttonType button, int elevator){
	printf("Enter deleteOrder\n");
	if (MASTER == 1){
		int i;
		for (i = 0; i < N_ORDERS; i++){
			if(orderQueue.Queue[i].dest == floor && orderQueue.Queue[i].buttonType == button && orderQueue.localPri[i] == elevator){
				orderQueue.inUse[i] = 0;
				orderQueue.localPri[i] = -1;
			}
		}
	}else{
		BufferInfo msg;
		encodeMessage(&msg, NULL, NULL, MSG_DELETE_ORDER, floor, button, 1);
		enqueue(sendQueue, &msg, sizeof(msg));
	}
	localQueue[floor] = 0;
}

int ordercmp(struct order *A, struct order *B){
	printf("Enter ordercmp\n");
	int x = 1;
	if (A->dest != B->dest) x = 0;
	if (A->buttonType != B->buttonType) x = 0;
	if (A->elevator != B->elevator) x = 0;
	return x;
}