#include <stdio.h>
#include "orderManager.h"
#include "publicTypes.h"
#include <pthread.h>

static struct backupQueue{
	Order Queue[N_ORDERS];
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
	printf("Init backup queue\n");
}

void addBackupOrder(Order storeOrder){
	pthread_mutex_lock(&(backupQueue.rwLock));
	int i = 0;
	int pos = N_ORDERS;
	for (i = 0; i < N_ORDERS; i++){
		if (backupQueue.inUse[i]){
			if (orderCompare(&storeOrder, &(backupQueue.Queue[i]))){
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
	if (storeOrder.buttonType == BUTTON_COMMAND){
		backupQueue.localPri[pos] = storeOrder.elevator;
	}else{
		backupQueue.localPri[pos] = -1;
	}
	pthread_mutex_unlock(&(backupQueue.rwLock));
	return;
}

void deleteBackupOrder(Order storeOrder){
	pthread_mutex_lock(&(backupQueue.rwLock));
	printf("Trying to delete in backup: floor: %d, button: %d, elev: %d\n", storeOrder.dest, storeOrder.buttonType, storeOrder.elevator);
	int i;
	for (i = 0; i < N_ORDERS; i++){
		if (backupQueue.inUse[i]){
			printf("In backup queue (pos %d): floor: %d, button: %d, elev: %d\n",i, backupQueue.Queue[i].dest, backupQueue.Queue[i].buttonType, backupQueue.Queue[i].elevator);
		}


		if (backupQueue.inUse[i] && backupQueue.Queue[i].dest == storeOrder.dest){ //Nesting for readability
			if (backupQueue.Queue[i].buttonType == storeOrder.buttonType && (backupQueue.Queue[i].elevator == storeOrder.elevator || backupQueue.localPri[i] == -1)){
				backupQueue.inUse[i] = 0;
				backupQueue.localPri[i] = -1;
				printf("Deleting backup: floor: %d, button: %d, elev: %d\n", backupQueue.Queue[i].dest, backupQueue.Queue[i].buttonType, backupQueue.Queue[i].elevator);
				//break;
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
			backupQueue.inUse[i] = 0;
		}
	}
	pthread_mutex_unlock(&(backupQueue.rwLock));
	return;
}