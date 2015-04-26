#ifndef _orderManager_
#define _orderManager_
#include "publicTypes.h"

void* orderManager(void *args);
int addNewOrder(struct Order newOrder);
int getNewOrder(int currentFloor, int nextFloor, int button);
void* sortMessages(void *args);
void deleteOrder(int floor, buttonType button, int elevator);
void reportElevState(int currentFloor, int nextFloor, int button);
void initPriorityQueue();
int orderCompare(struct Order *orderA, struct Order *orderB);
void importBackupOrders(struct Order backupOrder);

#endif