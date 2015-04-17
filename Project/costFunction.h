#include <stdio.h>
#include <pthread.h>


struct order{
	int dest;
	int buttonType;
	int elevator;
};
struct{
	struct order Queue[100];
	int inUse[100];
	int localPri[100];
	pthread_mutex_t rwLock;
}orderQueue;


void initPriorityQueue();
void addNewOrder(struct order newOrder, int currentFloor, int nextFloor);
int getNewOrder(int currentFloor);
