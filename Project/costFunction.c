#include "elevDriver.h"
#include "costFunction.h"
#include "network_modulev2.h"
#include <stdlib.h>
#include <pthread.h>



/*int findLowestCost(int priority[100] ,int inUse[100], struct order queue[100], int currentFloor, int nextFloor){
	//printf("Find lowest cost\n");
	int i, minPos,cost, backlog;
	int queuePos = 0;
	int min = N_FLOORS * 2;
	backlog = 0;
	for (i = 0; i < N_ORDERS; i++){
		if (inUse[i] == 1){// && (orderQueue.enRoute[i] == 0)){
			cost = findCost(queue[i].dest,currentFloor, nextFloor,queue[i].buttonType);
			//printf("Cost: %d, floor: %d Elev: %d  Input: current: %d, next: %d\n", cost, queue[i].dest,queue[i].elevator, currentFloor, nextFloor);
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
	//if (inUse[queuePos] == 1){
		//printf("Button lamp off2: floor: %d, type: %d\n", queue[queuePos].dest, queue[queuePos].buttonType);
	//	setButtonLamp(queue[queuePos].dest, queue[queuePos].buttonType, 0);
	//}
	//inUse[queuePos] = 0;
	//priority[queuePos] = -1;
	//queue[queuePos].elevator = 0;
	
	//printf("min cost: %d and pos: %d, backlog: %d\n",min,minPos, backlog);
	if (min == N_FLOORS*2){
		//printf("Dette burde skje\n");
		return -1;
	}else{
		//printf("Returnerer ordre: min: %d, pos: %d\n", min, minPos);
		//orderQueue.enRoute[minPos] = 1;
		return minPos;
	}
}*/
int findCost(int costFloor,int currentFloor, int nextFloor,int buttonType, int elevButton){
	int cost;
	int dir = nextFloor - currentFloor;
	cost = costFloor - currentFloor;
	//printf("findCost: current: %d, next: %d, cost: %d\n", currentFloor, nextFloor, cost);
	if ((elevButton == BUTTON_CALL_UP) && ((buttonType == BUTTON_CALL_DOWN) || (costFloor < currentFloor))){
		cost = (N_FLOORS - currentFloor) + (N_FLOORS - costFloor) - 2 + N_FLOORS;
	}else if ((elevButton == BUTTON_CALL_DOWN) && ((buttonType == BUTTON_CALL_UP) || (costFloor > currentFloor))){
		cost = costFloor + currentFloor + N_FLOORS;
	}else if (nextFloor == -1){
		cost = abs(cost);
		//printf("nextFloor = -1;\n");
	}else if ((buttonType == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_UP) && (currentFloor > costFloor)){
		cost = (N_FLOORS - currentFloor) + (N_FLOORS - costFloor) - 2 + N_FLOORS;
	}else if ((buttonType == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_DOWN) && (currentFloor < costFloor)){
		cost = costFloor + currentFloor + N_FLOORS;
	}else if (buttonType == BUTTON_COMMAND){
		cost = abs(cost);
	}else if (elevButton == buttonType){
		cost = abs(cost);
	}else if ((cost * dir) >= 0 && ((dir>0 && (buttonType == 0 || buttonType == 2))||(dir<0 && (buttonType == 2 || buttonType == 1)))){
		cost = abs(cost);
		printf("nextFloor right way\n");
	}else{
		printf("This should not happen!!!!!!!!!!!!!\n");
		cost = N_FLOORS + 1;
		printf("N_FLOORS+1\n" );
	}
	//printf("cost: %d, floor: %d, button: %d\n", cost, costFloor, buttonType);
	return cost;
}