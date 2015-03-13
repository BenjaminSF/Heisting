#include "elevDriver.h"
#include "costFunction.h"
#include <stdlib.h>

struct order{
	int from;
	int dest;
	int buttonType;
	int elevator
};
struct{
	struct order Queue[100];
	int inUse[100];
	int costOfQueue[100];
	int localPri[100];
}orderQueue;




/*int** priorityQueue;
void initPriorityQueue(int N_elevs, int N_floors){
	priorityQueue = malloc(N_elevs * sizeof(int *));
	int i;
	for (i = 0; i < N_floors; i++){
		priorityQueue[i] = malloc(N_floors * sizeof(int));
	}
	memset(priorityQueue, -1, sizeof(priorityQueue[0][0]) * N_floors * N_elevs);
}
//newFloorOrder[from to buttonType]
void setPriorityQueue(int** priorityQueue, int* newFloorOrder){

}*/

void addNewOrder(struct *orderQueue, struct order newOrder){
	int pos = 0;
	while(orderQueue->inUse[pos]){
		pos++
		if (pos == 100){
			fprintf("Error: orderQueue is full, order not received");
			return;
		}
	}
	orderQueue->Queue[pos]
}

int getNewOrder(int elevator){
	if internal_button_order//pseudo
		do that;
	else
		do lowestCost;

}


