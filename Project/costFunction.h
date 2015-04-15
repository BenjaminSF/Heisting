

struct order{
	int dest;
	int buttonType;
	int elevator
};
struct{
	struct order Queue[100];
	int inUse[100];
	int localPri[100];
	pthread_mutex_t rwLock;
}orderQueue;


void initPriorityQueue();
void addNewOrder(struct order newOrder);
int getNewOrder(int currentFloor);
int lowestCost(struct OrderQueue order,int currentFloor);
int findCost(struct order newOrder,int currentFloor);