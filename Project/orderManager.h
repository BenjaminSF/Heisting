#ifndef _orderManager_
#define _orderManager_

#include "elevDriver.h"
#include "costFunction.h"
#define N_ELEVATORS 5

extern struct orderQueueType orderQueue;
pthread_mutexattr_t orderQueuemattr;

void* orderManager(void *args);
int addNewOrder(struct order newOrder, int currentFloor, int nextFloor);
int getNewOrder(int currentFloor, int nextFloor, int button);
void* sortMessages(void *args);
void* masterTimeout(void *args);
void deleteOrder(int floor, buttonType button, int elevator);
void reportElevState(int currentFloor, int nextFloor, int button);
void initPriorityQueue();
int ordercmp(struct order *A, struct order *B);
void importBackupOrders(struct order x);
//void distributeOrders();

#endif