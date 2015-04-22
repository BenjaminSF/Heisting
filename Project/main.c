#include <pthread.h>
#include "mainDriver.h"
#include "network_modulev2.h"
#include "orderManager.h"
#define N_FLOORS 4
#define N_ELEVATORS 5


void main(){
	int masterStatus;
	masterStatus = init_network();
	if (masterStatus == -1){
		perror("Connection and master deciding failed\n");
	}else if(masterStatus == 0){
		printf("Backup\n");
	}else{
		printf("Master\n");
	}

	pthread_t driver, sendMessages, receiveMessages, manager;
	pthread_create(&driver,NULL,&mainDriver,NULL);
	pthread_create(&manager, NULL, &orderManager, NULL);
	pthread_create(&sendMessages, NULL, &send_message, 0);
	//struct ListenParams
	//pthread_create(&receiveMessages, NULL, &listen_for_messages, NULL);




	return;
}