#include <pthread.h>
#include "mainDriver.h"
#include "network_modulev2.h"
#include "orderManager.h"
#define N_FLOORS 4
#define N_ELEVATORS 5
int MASTER;

void main(){
	MASTER = init_network();
	if (MASTER == -1){
		perror("Connection and master deciding failed\n");
	}else if(MASTER == 0){
		printf("Backup\n");
	}else{
		printf("Master\n");
	}

	pthread_t driver, sendMessages, receiveMessages, manager, printsAreFun;
	pthread_create(&driver,NULL,&mainDriver,NULL);
	pthread_create(&manager, NULL, &orderManager, NULL);
	//pthread_create(&printsAreFun,NULL,&printFunction,NULL);
	//pthread_create(&sendMessages, NULL, &send_message, 0);

	pthread_join(driver,NULL);
	//pthread_join(sendMessages,NULL);
	printf("test\n");
	pthread_join(manager,NULL);
	//pthread_join(printsAreFun,NULL);
	//struct ListenParams
	//pthread_create(&receiveMessages, NULL, &listen_for_messages, NULL);




	return;
}