#include <pthread.h>
#include "mainDriver.h"
#include "networkModule.h"
#include "orderManager.h"
#include "elevDriver.h"
#include "publicTypes.h"

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

	int localIP = getLocalIP();
	pthread_t driver, sendMessages, receiveMessages, manager;
	pthread_create(&driver,NULL,&mainDriver,(void *) &localIP);
	pthread_create(&receiveMessages, 0, &listen_for_messages, 0);
	pthread_create(&sendMessages, 0, &send_message, 0);
	pthread_create(&manager, NULL, &orderManager, NULL);

	pthread_join(driver,NULL);
	pthread_join(sendMessages, NULL);
	pthread_join(receiveMessages, NULL);
	pthread_join(manager,NULL);

	return 0;
}