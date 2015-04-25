#include <stdio.h>
#include "orderManager.h"

static struct backupQueue{
	struct order Queue[N_ORDERS];
	int inUse[N_ORDERS];
	int localPri[N_ORDERS];
	pthread_mutex_t rwLock;
}backupQueue;

void initBackupQueue(){
	int i;
	for (i = 0; i < N_ORDERS; i++){
		backupQueue.inUse[i] = 0;
		backupQueue.localPri[i] = -1;
	}
	pthread_mutexattr_t backupAttr;
	pthread_mutexattr_init(&backupAttr);
	pthread_mutexattr_setpshared(&backupAttr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&(backupQueue.rwLock), &backupAttr);
	printf("Setup backup queue\n");
}

void addBackupOrder(int floor, int button, int elevator){
	struct order storeOrder;
	storeOrder.dest = floor;
	storeOrder.buttonType = button;
	storeOrder.elevator = elevator;
	pthread_mutex_lock(&(backupQueue.rwLock));
	int i = 0;
	int pos = N_ORDERS;
	for (i = 0; i < N_ORDERS; i++){
		if (backupQueue.inUse[i]){
			if (ordercmp(&storeOrder, &(backupQueue.Queue[i]))){
				pthread_mutex_unlock(&(backupQueue.rwLock));
				return;
			}
		}else if (pos > i){
			pos = i;
		}
	}
	if (pos == N_ORDERS){
		printf("Backup queue is full\n");
		pthread_mutex_unlock(&(backupQueue.rwLock));
		return;
	}

	backupQueue.Queue[pos] = storeOrder;
	backupQueue.inUse[pos] = 1;
	if (button == BUTTON_COMMAND){
		backupQueue.localPri[pos] = elevator;
	}else{
		backupQueue.localPri[pos] = -1;
	}
	pthread_mutex_unlock(&(backupQueue.rwLock));
	return;

}

void deleteBackupOrder(int floor, int button, int elevator){
	pthread_mutex_lock(&(backupQueue.rwLock));
	int i;
	for (i = 0; i < N_ORDERS; i++){
		if (backupQueue.inUse[i] && backupQueue.Queue[i].dest == floor){ //Nesting for readability
			if (backupQueue.Queue[i].buttonType == button && (backupQueue.Queue[i].elevator == elevator || backupQueue.localPri[i] == -1)){
				backupQueue.inUse[i] = 0;
				backupQueue.localPri[i] = -1;
				break; //There should not be any duplicates here, so this is fine
			}
		}
	}
	pthread_mutex_unlock(&(backupQueue.rwLock));
	return;
}

void transferBackupOrders(){
	printf("Transfer backup on new master\n");
	pthread_mutex_lock(&(backupQueue.rwLock));
	int i;
	for (i = 0; i < N_ORDERS; i++){
		if (backupQueue.inUse[i]){
			importBackupOrders(backupQueue.Queue[i]);
		}
	}
	pthread_mutex_unlock(&(backupQueue.rwLock));
	return;
}