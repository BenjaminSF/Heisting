#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include "orderManager.h"
#include "networkModule.h"
#include "mainDriver.h"
#include "costFunction.h"
#include "elevDriver.h"
#include "backupManager.h"
#include "publicTypes.h"

void distributeOrders();
void sendPriorityQueue(int dstAddr, int masterStatus);
void* orderTimeout();

struct orderQueueType{
	struct order Queue[N_ORDERS];
	int inUse[N_ORDERS];
	int localPri[N_ORDERS];
	int enRoute[N_ORDERS];
	int elevatorIP[N_ORDERS];
	pthread_mutex_t rwLock;
}orderQueue;

int localManQueue[N_FLOORS];
int localManButtons[N_FLOORS];
int localIPlist[N_FLOORS*N_BUTTONS];

struct {
	int active[N_ELEVATORS];
	int floor[N_ELEVATORS];
	int nextFloor[N_ELEVATORS];
	int button[N_ELEVATORS];
	int direction[N_ELEVATORS];
} elevStates;

sem_t timeoutSem;
static int bestProposal;

void* orderManager(void* args){
	printf("Master: %d\n", getMaster());
	struct timespec sleep = {.tv_sec = 2, .tv_nsec = 0};
	struct timespec rem;
	bestProposal = getLocalIP();
	sem_init(&timeoutSem, 0, 0);
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
			printf("I AM SLAVE\n");
			setMaster(0);
		}
		setMasterIP(bestProposal);
		bestProposal = getLocalIP();
	}
	pthread_join(orderTimeout_, NULL);
	pthread_join(sortMessages_, NULL);
	return NULL;
}

int addNewOrder(struct order newOrder){
	printf("Enter addNewOrder---------------------------------------------------\n");
	if (getMaster() == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		int pos = 0;
		struct order storeOrder;
		storeOrder.dest = newOrder.dest;
		storeOrder.buttonType = newOrder.buttonType;
		storeOrder.elevator = newOrder.elevator;
		
		while(orderQueue.inUse[pos]){
			if(orderCompare(&(orderQueue.Queue[pos]), &storeOrder)){
				pthread_mutex_unlock(&(orderQueue.rwLock));
				return -1;
			} 
			pos++;
			if (pos == N_ORDERS){
				printf("Error: orderQueue is full, order not received\n");
				pthread_mutex_unlock(&(orderQueue.rwLock));
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
			if (storeOrder.elevator == getLocalIP()){
				setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
			}//else{
			//	BufferInfo commandMsg;
			//	encodeMessage(&commandMsg, 0, storeOrder.elevator, MSG_SET_LAMP, storeOrder.dest, storeOrder.buttonType, 1);
			//}
		}		
		pthread_mutex_unlock(&(orderQueue.rwLock));
		BufferInfo backupMsg;
		encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_ADD, storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
		enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
	}else{ //Send new order to the master
		printf("Dette er en slave\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_ADD_ORDER, newOrder.dest, newOrder.buttonType, 0);
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor, int button){
	int destFloor = -1;
	if (getMaster() == 1){
		distributeOrders();
		int i;
		for (i = 0; i < N_FLOORS; i++){
			if (localManQueue[i] == 1){
				destFloor = i;
				localManQueue[i] = 0;
			}
		} 
	}else{
		int i;
		for (i = 0; i < N_FLOORS; i++){
			if (localManQueue[i] == 1){
				destFloor = i;
				localManQueue[i] = 0;
			}
		} 
	}
	return destFloor;
}

void distributeOrders(){ //Master only
	int addrsCount, i, j, tmpAddr, minCost, tmpCost, minFloor, minElev, minButton, minPos, minOrderPos;
	addrsCount = getAddrsCount();
	pthread_mutex_lock(&(orderQueue.rwLock));
	minCost = N_FLOORS * 4;
	for (j = 0; j < N_ORDERS; j++){
		for (i = 0; i < addrsCount; i++){
			tmpAddr = addrsList(i);
			if (orderQueue.inUse[j] && (orderQueue.localPri[j] == tmpAddr)){
				tmpCost = findCost(orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i], elevStates.direction[i]);
				if (tmpCost < minCost && !orderQueue.enRoute[j]){
					minCost = tmpCost;
					minFloor = orderQueue.Queue[j].dest;
					minElev = tmpAddr;
					minButton = orderQueue.Queue[j].buttonType;
					minPos = i;
					minOrderPos = j;
				}
			}else if (orderQueue.inUse[j] && (orderQueue.localPri[j] == -1)){// && !elevStates.active[i]){
				tmpCost = findCost(orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i], elevStates.direction[i]);
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
	pthread_mutex_unlock(&(orderQueue.rwLock));

	if (minCost < N_FLOORS){
		localIPlist[minFloor+minButton] = minElev;
		orderQueue.enRoute[minOrderPos] = 1;
		elevStates.active[minPos] = j;
		printf("Send order to: %d, floor: %d, cost: %d\n", minElev, minFloor, minCost);
		if (minElev == getLocalIP()){
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
	BufferInfo newMsg;
	struct order newBackupOrder;
	while(1){
		wait_for_content(receiveQueue);
		dequeue(receiveQueue, &bufOrder);
		int myState = bufOrder.myState;
		int dstAddr = bufOrder.dstAddr;
		int srcAddr = bufOrder.srcAddr;
		if ((myIP == dstAddr) || (broadcast == dstAddr)){
			switch(myState){
				case MSG_SET_LAMP:
					printf("Receive: MSG_SET_LAMP\n");
					setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
					break;
				case MSG_CONNECT_SEND:
					printf("Receive: MSG_CONNECT_SEND\n");
					addElevatorAddr(bufOrder.srcAddr);
					if (getMaster()) sendPriorityQueue(srcAddr, 1);
					encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, getMaster(), -1, -1);
					enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					break;
				case MSG_CONNECT_RESPONSE:
					if (bufOrder.masterStatus == 1){
						setMasterIP(srcAddr);
					}
					addElevatorAddr(srcAddr);
					break;
				case MSG_MASTER_REQUEST:
					printf("Receive: MSG_MASTER_REQUEST\n");
					resetAddrsList();
					int candidate = 1;
					if (srcAddr > myIP){
						candidate = 0;
					}
					encodeMessage(&newMsg, 0, 0, MSG_MASTER_PROPOSAL, candidate, -1, -1);
					enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					break;
				case MSG_MASTER_PROPOSAL:
					printf("Receive: MSG_MASTER_PROPOSAL\n");
					if (srcAddr > bestProposal){
						bestProposal = srcAddr;
					}
					addElevatorAddr(srcAddr);	
					break;
				case MSG_ADDR_REQUEST:
					printf("Receive: MSG_ADDR_REQUEST\n");
					BufferInfo newMsg;
					encodeMessage(&newMsg, 0, srcAddr, MSG_ADDR_RESPONSE,-1, -1, -1);
					enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					break;
				case MSG_ADDR_RESPONSE:
					printf("Receive: MSG_ADDR_RESPONSE\n");
					addElevatorAddr(srcAddr);
					break;
				case MSG_ADD_ORDER:
					if (getMaster() == 1){
						printf("Receive: MSG_ADD_ORDER\n");
						struct order newOrder;
						newOrder.dest = bufOrder.nextFloor;
						newOrder.buttonType = bufOrder.buttonType;
						newOrder.elevator = bufOrder.active;
						addNewOrder(newOrder);
						if (bufOrder.buttonType == BUTTON_COMMAND){
							encodeMessage(&newMsg, 0, bufOrder.active, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						}else{
							BufferInfo newMsg;
							encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						}
					}
					break;
				case MSG_DELETE_ORDER:
					if(getMaster() == 1){
						printf("Receive: MSG_DELETE_ORDER\n");
						deleteOrder(bufOrder.currentFloor,bufOrder.buttonType,srcAddr);
						if (bufOrder.buttonType != BUTTON_COMMAND){
							encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.currentFloor, bufOrder.buttonType, 0);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						}
					BufferInfo backupMsg;
					encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_DELETE, bufOrder.currentFloor, bufOrder.buttonType, srcAddr);
					enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
					}
					break;
				case MSG_ELEVSTATE:
					if(getMaster() == 1){
						printf("Receive: MSG_ELEVSTATE\n");
						int i;
						for (i = 0; i < getAddrsCount(); i++){
							if (addrsList(i) == srcAddr){
								elevStates.floor[i] = bufOrder.currentFloor;
								elevStates.nextFloor[i] = bufOrder.nextFloor;
								if (bufOrder.currentFloor == N_FLOORS-1) elevStates.direction[i] = -1;
								if (bufOrder.currentFloor == 0) elevStates.direction[i] = 1;
								if (bufOrder.nextFloor == -1) elevStates.active[i] = 0;
								elevStates.button[i] = bufOrder.buttonType;
								break;
							}
						}
					}
					break;
				case MSG_CONFIRM_ORDER:
					if(getMaster() == 1){	
						printf("Receive: MSG_CONFIRM_ORDER\n");
						int i;
						for (i = 0; i < N_ORDERS; i++){
							if (orderQueue.inUse[i] && (orderQueue.Queue[i].dest == bufOrder.nextFloor) && orderQueue.enRoute[i]){
								orderQueue.enRoute[i] = 1;
							}
						}
					}
					break;
				case MSG_IM_ALIVE:
					if(getMaster() == 0){
						printf("Receive: MSG_IM_ALIVE\n");
						sem_post(&timeoutSem);
					}else{
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
					break;
				case MSG_DO_ORDER:
					if (getMaster() == 0){
						printf("Receive: DO_ORDER%d\n", bufOrder.nextFloor);
						localManQueue[bufOrder.nextFloor] = 1;
						localManButtons[bufOrder.nextFloor] = bufOrder.buttonType;
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_CONFIRM_ORDER,bufOrder.nextFloor, -1, -1);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					}
					break;
				case MSG_BACKUP_ADD:
					if(getMaster() == 0){
					printf("Receive: MSG_BACKUP_ADD\n");
					newBackupOrder.dest = bufOrder.nextFloor;
					newBackupOrder.buttonType = bufOrder.buttonType;
					newBackupOrder.elevator = bufOrder.active;
					addBackupOrder(newBackupOrder);
					}
					break;
				case MSG_BACKUP_DELETE:
					if(getMaster() == 0){
					printf("Receive: MSG_BACKUP_DELETE\n");
					newBackupOrder.dest = bufOrder.nextFloor;
					newBackupOrder.buttonType = bufOrder.buttonType;
					newBackupOrder.elevator = bufOrder.active;
					deleteBackupOrder(newBackupOrder);
					}
					break;
				default:
					break;

			}
		}
	}
}

void* masterTimeout(void *args){
	printf("Enter timeout\n");
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
					if (button == BUTTON_COMMAND && elevator != getLocalIP()){
						encodeMessage(&newMsg, 0, elevator, MSG_SET_LAMP, floor, button, 0);
					}else if (button != BUTTON_COMMAND){
						encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, floor, button, 0);
						setButtonLamp(floor, button, 0);
					}else{
						setButtonLamp(floor, button, 0);
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
		struct order newBackupOrder;
		newBackupOrder.dest = floor;
		newBackupOrder.buttonType = button;
		newBackupOrder.elevator = elevator;
		deleteBackupOrder(newBackupOrder);
	}
}
int orderCompare(struct order *orderA, struct order *orderB){
	int check = 1;
	if (orderA->dest != orderB->dest) check = 0;
	if (orderA->buttonType != orderB->buttonType) check = 0;
	if (orderA->elevator != orderB->elevator) check = 0;
	return check;
}
void reportElevState(int currentFloor, int nextFloor, int button){
	printf("Reporting: current: %d, next: %d\n", currentFloor, nextFloor);
	if (getMaster() == 1){
		int i;
		int myIP = getLocalIP();
		for (i = 0; i < getAddrsCount(); i++){
			if (addrsList(i) == myIP){
				elevStates.floor[i] = currentFloor;
				elevStates.nextFloor[i] = nextFloor;
				if (nextFloor == -1){
					elevStates.active[i] = 0;
				}else{
					elevStates.active[i] = 1;
				}
				elevStates.button[i] = button;
				if (currentFloor == N_FLOORS -1) elevStates.direction[i] = -1;
				if (currentFloor == 0) elevStates.direction[i] = 1;
				break;
			}
		}
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
	initBackupQueue();
	printf("Init orderQueue and backupQueue\n");
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
				int pos = orderQueue.Queue[i].dest + orderQueue.Queue[i].buttonType;
				resetAddr(localIPlist[pos]);
				orderQueue.enRoute[i] = 0;

				//resetAddrsList();
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

void importBackupOrders(struct order backupOrder){
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
	orderQueue.Queue[i] = backupOrder;
	if (backupOrder.buttonType == BUTTON_COMMAND){
		orderQueue.localPri[i] = backupOrder.elevator;
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