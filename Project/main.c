#include <pthread.h>
#include "mainDriver.h"
#include "network_modulev2.h"
#include "orderManager.h"
#include "elevDriver.h"
#define N_FLOORS 4
#define N_ELEVATORS 5

int main(){
	int masterInput = init_network();
	if (masterInput == -1){
		perror("Connection and master deciding failed\n");
	}else if(masterInput == 0){
		printf("Backup\n");
	}else{
		printf("Master\n");
	}
	if (!elevDriver_initialize()) {
		printf("Unable to initialize elevator hardware!\n");
		return 0;
	}


	pthread_t driver, sendMessages, receiveMessages, manager;
	pthread_create(&driver,NULL,&mainDriver,NULL);
	pthread_create(&receiveMessages, 0, &listen_for_messages, 0);
	pthread_create(&sendMessages, 0, &send_message, 0);
	pthread_create(&manager, NULL, &orderManager, NULL);
	//pthread_create(&printsAreFun,NULL,&printFunction,NULL);
	//pthread_create(&sendMessages, NULL, &send_message, 0);

	pthread_join(driver,NULL);
	pthread_join(sendMessages, NULL);
	pthread_join(receiveMessages, NULL);
	//pthread_join(sendMessages,NULL);
	printf("test\n");
	pthread_join(manager,NULL);
	//pthread_join(printsAreFun,NULL);
	//struct ListenParams
	//pthread_create(&receiveMessages, NULL, &listen_for_messages, NULL);

	return 0;
}