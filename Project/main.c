#include <pthread.h>
#include "mainDriver.h"
#include "networkModule.h"
#include "orderManager.h"
#include "elevDriver.h"
#include "publicTypes.h"

int main(){
	int masterInput = initNetwork();
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
	pthread_t driver, sendMessages_, receiveMessages_, manager;
	pthread_create(&driver,NULL,&mainDriver,(void *) &localIP);
	pthread_create(&receiveMessages_, 0, &receiveMessages, 0);
	pthread_create(&sendMessages_, 0, &sendMessages, 0);
	pthread_create(&manager, NULL, &orderManager, NULL);

	pthread_join(driver,NULL);
	pthread_join(sendMessages_, NULL);
	pthread_join(receiveMessages_, NULL);
	pthread_join(manager,NULL);

	return 0;
}