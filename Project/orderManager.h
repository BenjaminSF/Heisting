#ifndef _orderManager_
#define _orderManager_

#include "elevDriver.h"
#include "costFunction.h"

void* orderManager(void *args);
int addNewOrder(struct order newOrder, int currentFloor, int nextFloor);
int getNewOrder(int currentFloor, int nextFloor);
void* sortMessages(void *args);
void* masterTimeout(void *args);
void deleteOrder(int floor, buttonType button);

#endif