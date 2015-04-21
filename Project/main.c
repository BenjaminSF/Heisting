
#include "mainDriver.h"
#include "network_modulev2.h"
#include "orderManager.h"
#define N_FLOORS 4
#define N_ELEVATORS 3
int MASTER;

void main(){
	MASTER = init_network();
	if (MASTER == -1){
		printf("Network initialization failed\n");
	}

	pthread_t driver, orderManager_;
	pthread_create(&driver,NULL,&mainDriver,NULL);
	pthread_create(&orderManager_, 0, &orderManager, 0);


	return;
}