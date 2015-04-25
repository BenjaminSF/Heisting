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
#include "backupManager.h"

void distributeOrders();
void sendPriorityQueue(int dstAddr, int masterStatus);
void* orderTimeout();

struct orderQueueType{
	struct order Queue[N_ORDERS];
	int inUse[N_ORDERS];
	int localPri[N_ORDERS];
	int enRoute[N_ORDERS];
	pthread_mutex_t rwLock;
}orderQueue;

int localManQueue[N_FLOORS];
int localManButtons[N_FLOORS];

struct {
	int active[N_ELEVATORS];
	int floor[N_ELEVATORS];
	int nextFloor[N_ELEVATORS];
	int button[N_ELEVATORS];
} elevStates;

sem_t timeoutSem;
static int bestProposal;
//int dummyMutex;

void* orderManager(void* args){

	//dummyMutex = 0;
	//initPriorityQueue();
	//int masterStatus = *(int *) args);
	//struct timespec startAnarchy;
	printf("Master: %d\n", getMaster());
	struct timespec sleep = {.tv_sec = 2, .tv_nsec = 0};
	struct timespec rem;
	bestProposal = getLocalIP();
	sem_init(&timeoutSem, 0, 0);
	//initPriorityQueue();
	pthread_t sortMessages_, masterTimeout_, orderTimeout_;

	pthread_create(&orderTimeout_, 0, &orderTimeout, 0);
	pthread_create(&sortMessages_, 0, &sortMessages, 0);
	while(1){
		pthread_create(&masterTimeout_, 0, &masterTimeout, 0);
		pthread_join(masterTimeout_, 0);
		nanosleep(&sleep, &rem);
		if (bestProposal == getLocalIP()){
			printf("I AM MASTER\n");
			setMaster(1);
		}else{
			printf("I am still slave :(\n");
			setMaster(0);
		}
		setMasterIP(bestProposal);
		bestProposal = getLocalIP();
	}
	pthread_join(orderTimeout_, NULL);
	pthread_join(sortMessages_, NULL);

	return NULL;
}

int addNewOrder(struct order newOrder, int currentFloor, int nextFloor){
	printf("Enter addNewOrder---------------------------------------------------\n");
	if (getMaster() == 1){
		//printf("Trying to get mutex\n");
		pthread_mutex_lock(&(orderQueue.rwLock));
		//dummyMutex++;
		//printf("Mutex owner: addNewOrder: %d\n", dummyMutex);
		int pos = 0;
		struct order storeOrder;
		storeOrder.dest = newOrder.dest;
		storeOrder.buttonType = newOrder.buttonType;
		storeOrder.elevator = newOrder.elevator;
		
		while(orderQueue.inUse[pos]){
			if(ordercmp(&(orderQueue.Queue[pos]), &storeOrder)){
				pthread_mutex_unlock(&(orderQueue.rwLock));
				//dummyMutex--;
				//printf("Add unlock mutex: %d\n", dummyMutex);
				return -1;
			} 
			pos++;
			if (pos == N_ORDERS){
				printf("Error: orderQueue is full, order not received\n");
				pthread_mutex_unlock(&(orderQueue.rwLock));
				//dummyMutex--;
				//printf("Add unlock mutex: %d\n", dummyMutex);
				return -1;
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
		orderQueue.enRoute[pos] = 0;
		if (storeOrder.buttonType == BUTTON_COMMAND){
			orderQueue.localPri[pos] = storeOrder.elevator;
			printf("add local order from: %d\n", storeOrder.elevator);
			setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
		}
		//printf("Mutex released: addNewOrder\n");
		
		pthread_mutex_unlock(&(orderQueue.rwLock));
		BufferInfo backupMsg;
		encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_ADD, storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
		enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
		//dummyMutex--;
		//printf("Add unlock mutex, %d\n", dummyMutex);
	}else{ //Send new order to the master
		printf("Dette er en slave\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_ADD_ORDER, newOrder.dest, newOrder.buttonType, 0);
		newMsg.currentFloor = currentFloor;
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor, int button){
	//printf("Enter getNewOrder: currentFloor: %d, nextFloor: %d\n", currentFloor, nextFloor);
	int destFloor = -1;
	//int dir = nextFloor - currentFloor;
	//if (nextFloor == -1) dir = 0;
	if (getMaster() == 1){
		//pthread_mutex_lock(&(orderQueue.rwLock));
		//destFloor = findLowestCost(orderQueue.localPri,orderQueue.inUse,orderQueue.Queue,currentFloor, nextFloor);
		//pthread_mutex_unlock(&(orderQueue.rwLock));
		distributeOrders();
		int i;
		//int minCost = N_FLOORS * 2;
		for (i = 0; i < N_FLOORS; i++){
			if (localManQueue[i] == 1){
				//tmp = findCost(i, currentFloor, nextFloor, localManButtons[i], button);
				//if (tmp < minCost){
				//	minCost = tmp;
				//	destFloor = i;
				//}
				destFloor = i;
				localManQueue[i] = 0;
			}
		} 
	}else{
		int i;
		//int minCost = N_FLOORS * 2;
		for (i = 0; i < N_FLOORS; i++){
			if (localManQueue[i] == 1){
				//tmp = findCost(i, currentFloor, nextFloor, localManButtons[i], button);
				//if (tmp < minCost){
				//	minCost = tmp;
				//	destFloor = i;
				//}
				destFloor = i;
				localManQueue[i] = 0;
			}
		} 
		//printf("Slave go to: %d\n", destFloor);
	}
	//printf("currentFloor: %d, nextFloor: %d, destFloor: %d\n", currentFloor, nextFloor, destFloor);

	return destFloor;
}

void distributeOrders(){ //Master only
	//printf("Enter distributeOrders\n");
	int addrsCount, i, j, tmpAddr, minCost, tmpCost, minFloor, minElev, minButton, minPos, minOrderPos;

	addrsCount = getAddrsCount();
	//printf("Getting mutex\n");
	pthread_mutex_lock(&(orderQueue.rwLock));
	//printf("Mutex owner: distributeOrders\n");
	minCost = N_FLOORS * 2;
	for (j = 0; j < N_ORDERS; j++){
		//if (orderQueue.inUse[j]) printf("Det finnes orders!\n");
		for (i = 0; i < addrsCount; i++){
			tmpAddr = addrsList(i);
			if (orderQueue.inUse[j] && (orderQueue.localPri[j] == tmpAddr)){// && !elevStates.active[i]){ //Send BUTTON_COMMAND orders first
				//printf("BUTTON_COMMAND, source: %d\n", tmpAddr);
				tmpCost = findCost(orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i]);
				//printf("tmpCostButton: %d, to: %d, from: %d, going to: %d, orderButton: %d, elevButton: %d\n", tmpCost, orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i]);
				if (tmpCost < minCost && !orderQueue.enRoute[j]){
					minCost = tmpCost;
					minFloor = orderQueue.Queue[j].dest;
					minElev = tmpAddr;
					minButton = orderQueue.Queue[j].buttonType;
					minPos = i;
					minOrderPos = j;
				}
				
			}else if (orderQueue.inUse[j] && (orderQueue.localPri[j] == -1)){// && !elevStates.active[i]){
				//printf("floor: %d, nextElevState: %d\n", elevStates.floor[i], elevStates.nextFloor[i]);
				tmpCost = findCost(orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i]);
				//printf("tmpCost: %d, to: %d, from: %d, going to: %d, orderButton: %d, elevButton: %d\n", tmpCost, orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i]);
				//tmpCost += orderQueue.enRoute[j] + elevStates.active[i];
				//printf("tmpCost: %d\n", tmpCost);
				if (tmpCost < minCost && !orderQueue.enRoute[j]){
					minCost = tmpCost;
					minFloor = orderQueue.Queue[j].dest;
					minElev = tmpAddr;
					minButton = orderQueue.Queue[j].buttonType;
					minPos = i;
					minOrderPos = j;
				}
			}
		}
		if (minCost == 0) break;
	}
	//printf("minCost: %d\n", minCost);
	pthread_mutex_unlock(&(orderQueue.rwLock));
	//printf("Release mutex\n");
	if (minCost < N_FLOORS){
		//printf("Sending elevator: %d, to floor: %d\n", minElev, minFloor);
		orderQueue.enRoute[minOrderPos] = 1;
		elevStates.active[minPos] = 1;
		printf("Send order to: %d, floor: %d, cost: %d\n", minElev, minFloor, minCost);
		if (minElev == getLocalIP()){
			//printf("Go here!\n");
			localManQueue[minFloor] = 1;
		}else{
			BufferInfo newMsg;
			encodeMessage(&newMsg, 0, minElev, MSG_DO_ORDER, minFloor, minButton, -1);
			enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		}
	}


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

			if (myState == MSG_SET_LAMP){
				printf("Receive: MSG_SET_LAMP\n");
				setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
			}
			if (myState == MSG_CONNECT_SEND){
				printf("Receive: MSG_CONNECT_SEND\n");
				addElevatorAddr(bufOrder.srcAddr);
				if (getMaster()) sendPriorityQueue(srcAddr, 1);
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, getMaster(), -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
			if (myState == MSG_CONNECT_RESPONSE){
				if (bufOrder.masterStatus == 1){

					setMasterIP(srcAddr);
				}
				addElevatorAddr(srcAddr);
			}
			if (myState == MSG_MASTER_REQUEST){
				printf("Receive: MSG_MASTER_REQUEST\n");
				resetAddrsList();
				int candidate = 1;
				if (srcAddr > myIP){
					candidate = 0;
				}
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_MASTER_PROPOSAL, candidate, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
			if (myState == MSG_MASTER_PROPOSAL){
				printf("Receive: MSG_MASTER_PROPOSAL\n");
				if (srcAddr > bestProposal){
					bestProposal = srcAddr;
				}
				addElevatorAddr(srcAddr);
			}
			if (myState == MSG_ADDR_REQUEST){
				printf("Receive: MSG_ADDR_REQUEST\n");
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, srcAddr, MSG_ADDR_RESPONSE,-1, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
			if (myState == MSG_ADDR_RESPONSE){
				printf("Receive: MSG_ADDR_RESPONSE\n");
				addElevatorAddr(srcAddr);
			}

			if (getMaster() == 1){
				if (myState == MSG_ADD_ORDER){
					printf("Receive: MSG_ADD_ORDER\n");
					struct order newOrder;
					newOrder.dest = bufOrder.nextFloor;
					newOrder.buttonType = bufOrder.buttonType;
					newOrder.elevator = bufOrder.active;
					addNewOrder(newOrder, bufOrder.currentFloor,bufOrder.nextFloor);
					if (bufOrder.buttonType == BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, bufOrder.active, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					}else{
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					}
				}
				if(myState == MSG_DELETE_ORDER){
					printf("Receive: MSG_DELETE_ORDER\n");
					deleteOrder(bufOrder.currentFloor,bufOrder.buttonType,srcAddr);
					if (bufOrder.buttonType != BUTTON_COMMAND){
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.currentFloor, bufOrder.buttonType, 0);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					}
					BufferInfo backupMsg;
					encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_DELETE, bufOrder.currentFloor, bufOrder.buttonType, srcAddr);
					enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
				}
				if (myState == MSG_ELEVSTATE){
					printf("Receive: MSG_ELEVSTATE\n");
					int i;
					for (i = 0; i < getAddrsCount(); i++){
						if (addrsList(i) == srcAddr){
							elevStates.floor[i] = bufOrder.currentFloor;
							elevStates.nextFloor[i] = bufOrder.nextFloor;
							if (bufOrder.nextFloor == -1) elevStates.active[i] = 0;
							elevStates.button[i] = bufOrder.buttonType;
							break;
						}
					}
				}
				if (myState == MSG_CONFIRM_ORDER){
					printf("Receive: MSG_CONFIRM_ORDER\n");
					int i;
					for (i = 0; i < N_ORDERS; i++){
						if (orderQueue.inUse[i] && (orderQueue.Queue[i].dest == bufOrder.nextFloor) && orderQueue.enRoute[i]){
							orderQueue.enRoute[i] = 1;
						}
					}
				}
				if(myState == MSG_IM_ALIVE){
					if (srcAddr != myIP){ // Master conflict
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_MASTER_REQUEST, -1, -1, -1);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						if (getLocalIP() != bestProposal){ // Give up master status
							sendPriorityQueue(bestProposal, 0);
							setMasterIP(bestProposal);
							setMaster(0);
						}
					}
				}
			
			}else{
				if (myState == MSG_IM_ALIVE){
					printf("Receive: MSG_IM_ALIVE\n");
					sem_post(&timeoutSem);
				}
				if (myState == MSG_DO_ORDER){
					printf("Receive: DO_ORDER%d\n", bufOrder.nextFloor);
					localManQueue[bufOrder.nextFloor] = 1;
					localManButtons[bufOrder.nextFloor] = bufOrder.buttonType;
					BufferInfo newMsg;
					encodeMessage(&newMsg, 0, 0, MSG_CONFIRM_ORDER,bufOrder.nextFloor, -1, -1);
					enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
				}
				if (myState == MSG_BACKUP_ADD){
					printf("Receive: MSG_BACKUP_ADD\n");
					addBackupOrder(bufOrder.nextFloor, bufOrder.buttonType, bufOrder.active);
				}
				if (myState == MSG_BACKUP_DELETE){
					printf("Receive: MSG_BACKUP_DELETE\n");
					deleteBackupOrder(bufOrder.nextFloor, bufOrder.buttonType, bufOrder.active);
				}
			}
		}
	}
}

void* masterTimeout(void *args){
	printf("Enter timeout\n");
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	struct timespec ts, rem;
	int masterStatus = getMaster();

	if (masterStatus == 0){
		printf("Timer: slave\n");
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec = ts.tv_sec + 6;
		int test;
		while(getMaster() == 0){
			test = sem_timedwait(&timeoutSem, &ts);
			if (test == -1){
				printf("Master timeout\n");
				resetAddrsList();
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_MASTER_REQUEST, -1, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
				return NULL;
			}
			//printf("Aliveness confirmed\n");
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec = ts.tv_sec + 15;
		}
	}else{
		printf("Timer: master\n");
		transferBackupOrders();
		reportElevState(getFloor(), -1, BUTTON_COMMAND);
		ts.tv_sec = 5;
		ts.tv_nsec = 0;
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_IM_ALIVE, 1, -1, -1);
		while(getMaster() == 1){
			enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			nanosleep(&ts, &rem);

		}
	}
	return NULL;
}

void deleteOrder(int floor, buttonType button, int elevator){
	printf("Enter deleteOrder: floor: %d, button: %d, elev: %d\n", floor, button, elevator);
	if (getMaster() == 1){
		int i;
		int remainingOrders = 0;
		for (i = 0; i < N_ORDERS; i++){
			if (orderQueue.inUse[i] == 1){
				remainingOrders++;
				if (orderQueue.Queue[i].dest == floor && orderQueue.Queue[i].buttonType == button && (orderQueue.localPri[i] == elevator || orderQueue.localPri[i] == -1)){
					orderQueue.inUse[i] = 0;
					orderQueue.localPri[i] = -1;
					orderQueue.enRoute[i] = 0;
					printf("Deleting order!\n");
					BufferInfo newMsg;
					if (button == BUTTON_COMMAND){
						encodeMessage(&newMsg, 0, elevator, MSG_SET_LAMP, floor, button, 0);
					}else{
						encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, floor, button, 0);
					}
					enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					remainingOrders--;
				}
			}
		}
		printf("remainingOrders: %d\n", remainingOrders);
	}else{
		BufferInfo msg;
		encodeMessage(&msg, 0, 0, MSG_DELETE_ORDER, floor, button, 1);
		enqueue(sendQueue, &msg, sizeof(BufferInfo));
	}
	setButtonLamp(floor, button, 0);
	localManQueue[floor] = 0;
}

int ordercmp(struct order *A, struct order *B){
	//printf("Enter ordercmp\n");
	int x = 1;
	if (A->dest != B->dest) x = 0;
	if (A->buttonType != B->buttonType) x = 0;
	if (A->elevator != B->elevator) x = 0;
	return x;
}

void reportElevState(int currentFloor, int nextFloor, int button){
	printf("Reporting: current: %d, next: %d\n", currentFloor, nextFloor);
	if (getMaster() == 1){
		elevStates.floor[0] = currentFloor;
		elevStates.nextFloor[0] = nextFloor;
		if (nextFloor == -1){
			elevStates.active[0] = 0;
		}else{
			elevStates.active[0] = 1;
		}
		elevStates.button[0] = button;
	}else{
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_ELEVSTATE, currentFloor, nextFloor, button);
		enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
	}
	
}

void initPriorityQueue(){
	int i;
	for (i = 0; i < N_ORDERS; i++){
		orderQueue.inUse[i] = 0;
		orderQueue.localPri[i] = -1;
		orderQueue.Queue[i].dest = 100;
		orderQueue.enRoute[i] = 0;

	}

	pthread_mutexattr_init(&orderQueuemattr);
	pthread_mutexattr_setpshared(&orderQueuemattr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&(orderQueue.rwLock), &orderQueuemattr);
	printf("Setup priority queue\n");
	initBackupQueue();
}

void* orderTimeout(){
	struct timespec ts, rem;
	int i;
	while(1){
		for (i = 0; i < N_ORDERS; i++){
			if (orderQueue.inUse[i] && orderQueue.enRoute[i]){
				orderQueue.enRoute[i]++;
			}
			if (orderQueue.enRoute[i] > 12){
				printf("Order timed out!!\n");
				orderQueue.enRoute[i] = 0;
				resetAddrsList();
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_ADDR_REQUEST, -1, -1, -1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
		}
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		nanosleep(&ts, &rem);
	}
	return NULL;
}

void importBackupOrders(struct order x){
	int i = 0;
	pthread_mutex_lock(&(orderQueue.rwLock));
	while(orderQueue.inUse[i]){
		i++;
		if (i == N_ORDERS){
			pthread_mutex_unlock(&(orderQueue.rwLock));
			return;
		}
	}
	orderQueue.inUse[i] = 1;
	orderQueue.Queue[i] = x;
	if (x.buttonType == BUTTON_COMMAND){
		orderQueue.localPri[i] = x.elevator;
	}else{
		orderQueue.localPri[i] = -1;
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
	return;
}

void sendPriorityQueue(int dstAddr, int masterStatus){
	printf("Send copy of priorityQueue to new slave\n");
	pthread_mutex_lock(&(orderQueue.rwLock));
	int i;
	BufferInfo msg;
	for (i = 0; i < N_ORDERS; i++){
		if (orderQueue.inUse[i]){
			if (!masterStatus){
				encodeMessage(&msg, 0, dstAddr, MSG_ADD_ORDER, orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, orderQueue.Queue[i].elevator);
			}else{
				encodeMessage(&msg, 0, dstAddr, MSG_BACKUP_ADD, orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, orderQueue.Queue[i].elevator);
			}
			enqueue(sendQueue, &msg, sizeof(BufferInfo));
		}
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
	return;
}