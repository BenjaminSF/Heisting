
#include "mainDriver.h"
#include "network_modulev2.h"
#define N_FLOORS 4
#define N_ELEVATORS 3


void main(){
	int masterStatus;
	masterStatus = init_network();

	pthread_t driver;
	pthread_create(&driver,NULL,&mainDriver,NULL);



	return;
}