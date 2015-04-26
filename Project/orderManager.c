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

// Private functions
void distributeOrders();
void sendPriorityQueue(int dstAddr, int masterStatus);
void* orderTimeout();
void* masterTimeout(void *args);

static struct{
	Order Queue[N_ORDERS];
	int inUse[N_ORDERS];
	int localPri[N_ORDERS];
	int enRoute[N_ORDERS];
	int elevatorIP[N_ORDERS];
	pthread_mutex_t rwLock;
}orderQueue;

int localManQueue[N_FLOORS];
int localManButtons[N_FLOORS];
int localIPlist[N_FLOORS*N_BUTTONS];

static struct {
	int floor[N_ELEVATORS];
	int nextFloor[N_ELEVATORS];
	int button[N_ELEVATORS];
} elevStates;

static sem_t timeoutSem;
static int bestProposal;

void* orderManager(void* args){
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
		setMasterIP(bestProposal);
		bestProposal = getLocalIP();
	}
	pthread_join(orderTimeout_, NULL);
	pthread_join(sortMessages_, NULL);
	return NULL;
}

int addNewOrder(Order storeOrder){
	if (getMasterStatus() == 1){
		pthread_mutex_lock(&(orderQueue.rwLock));
		int pos = 0;
		//Order storeOrder;
		//storeOrder.dest = newOrder.dest;
		//storeOrder.buttonType = newOrder.buttonType;
		//storeOrder.elevator = newOrder.elevator;
		
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
		orderQueue.Queue[pos] = storeOrder;
		orderQueue.inUse[pos] = 1;
		orderQueue.enRoute[pos] = 0;
		
		if (storeOrder.buttonType != BUTTON_COMMAND){
			setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
			BufferInfo newMsg;
			encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, storeOrder.dest, storeOrder.buttonType, 1);
			enqueue(sendQueue, &newMsg, sizeof(newMsg));
		}else if (storeOrder.buttonType == BUTTON_COMMAND){
			orderQueue.localPri[pos] = storeOrder.elevator;
			if (storeOrder.elevator == getLocalIP()){
				setButtonLamp(storeOrder.dest,storeOrder.buttonType,1);
			}else{
				BufferInfo commandMsg;
				encodeMessage(&commandMsg, 0, storeOrder.elevator, MSG_SET_LAMP, storeOrder.dest, storeOrder.buttonType, 1);
				enqueue(sendQueue, &commandMsg, sizeof(BufferInfo));
			}
		}		
		pthread_mutex_unlock(&(orderQueue.rwLock));
		BufferInfo backupMsg;
		encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_ADD, storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
		enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
	}else{ //Send new order to the master
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_ADD_ORDER, storeOrder.dest, storeOrder.buttonType, 0);
		enqueue(sendQueue, &newMsg, sizeof(newMsg));
	}
	return -1;
}

int getNewOrder(int currentFloor, int nextFloor, int button){
	int destFloor = -1;
	if (getMasterStatus() == 1){
		distributeOrders();
	}
	int i;
	for (i = 0; i < N_FLOORS; i++){
		if (localManQueue[i] == 1){
			destFloor = i;
			localManQueue[i] = 0;
		}
	} 
	return destFloor;
}

void distributeOrders(){ //Master only
	int addrsCount, i, j, tmpAddr, minCost, tmpCost, minFloor, minElev, minButton, minOrderPos;
	addrsCount = getAddrsCount();
	pthread_mutex_lock(&(orderQueue.rwLock));
	minCost = N_FLOORS * 4;
	for (j = 0; j < N_ORDERS; j++){
		for (i = 0; i < addrsCount; i++){
			tmpAddr = addrsList(i);
			if (orderQueue.inUse[j] && ((orderQueue.localPri[j] == tmpAddr) || (orderQueue.localPri[j] == -1))){
				tmpCost = findCost(orderQueue.Queue[j].dest, elevStates.floor[i], elevStates.nextFloor[i], orderQueue.Queue[j].buttonType, elevStates.button[i]);
				if (tmpCost < minCost && !orderQueue.enRoute[j]){
					minCost = tmpCost;
					minFloor = orderQueue.Queue[j].dest;
					minElev = tmpAddr;
					minButton = orderQueue.Queue[j].buttonType;
					minOrderPos = j;
				}
			}
		}
		if (minCost == 0) break;
	}

	if (minCost < N_FLOORS *2){ // Threshold for sending orders
		localIPlist[N_BUTTONS*minFloor+minButton] = minElev;
		orderQueue.enRoute[minOrderPos] = 1;
		if (minElev == getLocalIP()){
			localManQueue[minFloor] = 1;
		}else{
			BufferInfo newMsg;
			encodeMessage(&newMsg, 0, minElev, MSG_DO_ORDER, minFloor, minButton, -1);
			enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		}
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
}

void* sortMessages(void *args){
	printf("Enter sortMessages\n");
	BufferInfo bufOrder;
	int myIP = getLocalIP();
	int broadcast = getBroadcastIP();
	BufferInfo newMsg;
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
					if (getMasterStatus()) sendPriorityQueue(srcAddr, 1);
					encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, getMasterStatus(), -1, -1);
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
					encodeMessage(&newMsg, 0, 0, MSG_MASTER_PROPOSAL, -1, -1, -1);
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
					if (getMasterStatus() == 1){
						printf("Receive: MSG_ADD_ORDER\n");
						printf("floor: %d, button: %d, elev: %d\n", bufOrder.nextFloor, bufOrder.buttonType, bufOrder.active);
						Order newOrder;
						newOrder.dest = bufOrder.nextFloor;
						newOrder.buttonType = bufOrder.buttonType;
						newOrder.elevator = bufOrder.active;
						addNewOrder(newOrder);
						/*if (bufOrder.buttonType == BUTTON_COMMAND){
							encodeMessage(&newMsg, 0, bufOrder.active, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						}else{
							BufferInfo newMsg;
							encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
						}*/
					}
					break;
				case MSG_DELETE_ORDER:
					if(getMasterStatus() == 1){
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
					if(getMasterStatus() == 1){
						printf("Receive: MSG_ELEVSTATE\n");
						int i;
						for (i = 0; i < getAddrsCount(); i++){
							if (addrsList(i) == srcAddr){
								elevStates.floor[i] = bufOrder.currentFloor;
								elevStates.nextFloor[i] = bufOrder.nextFloor;
								elevStates.button[i] = bufOrder.buttonType;
								break;
							}
						}
					}
					break;
				case MSG_CONFIRM_ORDER:
					if(getMasterStatus() == 1){	
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
					if(getMasterStatus() == 0){
						printf("Receive: MSG_IM_ALIVE\n");
						sem_post(&timeoutSem);
					}else{
						if (srcAddr != myIP){ // Master conflict
							BufferInfo newMsg;
							encodeMessage(&newMsg, 0, 0, MSG_MASTER_REQUEST, -1, -1, -1);
							enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
							if (getLocalIP() != bestProposal){ // Give up master status
								setMasterIP(bestProposal);
								sendPriorityQueue(bestProposal, 0);
								BufferInfo getOrdersMsg;
								encodeMessage(&getOrdersMsg, 0, 0, MSG_CONNECT_SEND, 0, -1, -1);
								enqueue(sendQueue, &getOrdersMsg, sizeof(BufferInfo));
							}
						}		
					}
					break;
				case MSG_DO_ORDER:
					if (getMasterStatus() == 0){
						printf("Receive: DO_ORDER%d\n", bufOrder.nextFloor);
						localManQueue[bufOrder.nextFloor] = 1;
						localManButtons[bufOrder.nextFloor] = bufOrder.buttonType;
						BufferInfo newMsg;
						encodeMessage(&newMsg, 0, 0, MSG_CONFIRM_ORDER,bufOrder.nextFloor, -1, -1);
						enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
					}
					break;
				case MSG_BACKUP_ADD:
					if(getMasterStatus() == 0){
					printf("Receive: MSG_BACKUP_ADD\n");
					Order newBackupOrder;
					newBackupOrder.dest = bufOrder.nextFloor;
					newBackupOrder.buttonType = bufOrder.buttonType;
					newBackupOrder.elevator = bufOrder.active;
					addBackupOrder(newBackupOrder);
					}
					break;
				case MSG_BACKUP_DELETE:
					if(getMasterStatus() == 0){
					printf("Receive: MSG_BACKUP_DELETE\n");
					Order newBackupOrder;
					newBackupOrder.dest = bufOrder.currentFloor;
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
	int masterStatus = getMasterStatus();
	if (masterStatus == 0){
		printf("Timer: slave\n");
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec = ts.tv_sec + 6;
		int test;
		while(getMasterStatus() == 0){
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
		while(getMasterStatus() == 1){
			enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			nanosleep(&ts, &rem);
			printf("Addresses in list: %d\n", getAddrsCount());
		}
	}
	return NULL;
}

void deleteOrder(int floor, buttonType button, int elevator){
	printf("Enter deleteOrder: floor: %d, button: %d, elev: %d\n", floor, button, elevator);
	if (getMasterStatus() == 1){
		int i;
		int remainingOrders = 0;
		for (i = 0; i < N_ORDERS; i++){
			if (orderQueue.inUse[i] == 1){
				remainingOrders++;
				printf("In queue: floor: %d, button: %d, elev: %d\n", orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, orderQueue.Queue[i].elevator);
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
					BufferInfo backupMsg;
					encodeMessage(&backupMsg, 0, 0, MSG_BACKUP_DELETE, floor, button, elevator);
					enqueue(sendQueue, &backupMsg, sizeof(BufferInfo));
				}
			}
		}
		printf("remainingOrders: %d\n", remainingOrders);
	}else{
		BufferInfo msg;
		encodeMessage(&msg, 0, 0, MSG_DELETE_ORDER, floor, button, 1);
		enqueue(sendQueue, &msg, sizeof(BufferInfo));
		Order newBackupOrder;
		newBackupOrder.dest = floor;
		newBackupOrder.buttonType = button;
		newBackupOrder.elevator = elevator;
		setButtonLamp(floor, button, 0);
		deleteBackupOrder(newBackupOrder);
	}
}

int orderCompare(Order *orderA, Order *orderB){
	int check = 1;
	if (orderA->dest != orderB->dest) check = 0;
	if (orderA->buttonType != orderB->buttonType) check = 0;
	if (orderA->elevator != orderB->elevator) check = 0;
	return check;
}

void reportElevState(int currentFloor, int nextFloor, int button){
	if (getMasterStatus() == 1){
		int i;
		int myIP = getLocalIP();
		for (i = 0; i < getAddrsCount(); i++){
			if (addrsList(i) == myIP){
				elevStates.floor[i] = currentFloor;
				elevStates.nextFloor[i] = nextFloor;
				elevStates.button[i] = button;
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

	pthread_mutex_init(&(orderQueue.rwLock), NULL);
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
				int pos = N_BUTTONS*orderQueue.Queue[i].dest + orderQueue.Queue[i].buttonType;
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

void importBackupOrders(Order backupOrder){
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
	printf("Send copy of priorityQueue\n");
	pthread_mutex_lock(&(orderQueue.rwLock));
	int i;
	BufferInfo msg;
	for (i = 0; i < N_ORDERS; i++){
		if (orderQueue.inUse[i]){
			if (!masterStatus){
				encodeMessage(&msg, 0, dstAddr, MSG_ADD_ORDER, orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, orderQueue.Queue[i].elevator);
			}else{
				if (orderQueue.Queue[i].buttonType != BUTTON_COMMAND || orderQueue.Queue[i].elevator == dstAddr){
					BufferInfo lightMsg;
					encodeMessage(&lightMsg, 0, dstAddr, MSG_SET_LAMP, orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, 1);
					enqueue(sendQueue, &lightMsg, sizeof(BufferInfo));
				}
				encodeMessage(&msg, 0, dstAddr, MSG_BACKUP_ADD, orderQueue.Queue[i].dest, orderQueue.Queue[i].buttonType, orderQueue.Queue[i].elevator);
			}
			enqueue(sendQueue, &msg, sizeof(BufferInfo));
		}
	}
	pthread_mutex_unlock(&(orderQueue.rwLock));
	return;
}