

#include "elevDriver.h"
#include "costFunction.h"
#include <stdio.h>


#define N_FLOORS 4



int main() {

// Initialize hardware

	if (!elevDriver_initialize()) {
		printf("Unable to initialize elevator hardware!\n");
		return 1;
}
	printf("Press STOP button to stop elevator and exit program.\n");
	setMotorDirection(DIRN_STOP);
	
	int nextFloor = 0;
	int i;
	int queueActive = 0;
	int currentFloor;
	int check;
	int maxCheck = 5;
	while(!isStopped()|| !isObstructed()){
		check = 0;
		currentFloor = -1;
		while(currentFloor == -1){
			currentFloor = getFloor();
			check++;
			if(isObstructed() || isStopped() || check == maxCheck){
				break;
			}
		}

		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND)){
				nextFloor = i;
				queueActive = 1;
			}
		}

		if(queueActive){

			//addNewOrder

			//int nextFloor = ..//getNeworder
			if(currentFloor == nextFloor){
				setMotorDirection(DIRN_STOP);
			}else{
				printf("Heis kjører fra %d til %d\n",currentFloor, nextFloor);
				setFloor(nextFloor);
		}
		}
		queueActive = 0;

	}
	
	setMotorDirection(DIRN_STOP);
	if(isObstructed()){
		printf("Elevator was obstructed\n");
	}
	if(isStopped()){
		setStopLamp(1);
		printf("Elevator was stopped\n");
		if()
	}
	if(check >= maxCheck){
		printf("Checked getFloor() %d times without sucess, and therefore stopped\n",check);
	}

	
return 0;
}
