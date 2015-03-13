

#include "elevDriver.h"
//#include "costFunction.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


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
	int i, j,k;
	int queueActive = 0;
	int currentFloor;
	int check;
	int maxCheck = 5;
	motorDirection direction;
	buttonType buttonCall;
	while(!isStopped()|| !isObstructed()){
		int localQueue[N_FLOORS];
		memset(localQueue,0,sizeof(int)*N_FLOORS);
		check = 0;
		currentFloor = getFloor();
		while(currentFloor == -1){
			check++;
			if(isObstructed() || isStopped() || check == maxCheck){
				break;
			}
			currentFloor = getFloor();
		}

		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND)){
				if(i != currentFloor){
					nextFloor = i;
					queueActive = 1;
			}
			}
		}

		if(queueActive){
			if(nextFloor-getFloor()> 0){
				direction = DIRN_UP;
				buttonCall = BUTTON_CALL_UP;
			}else{
				direction = DIRN_DOWN;
				buttonCall = BUTTON_CALL_DOWN;
			}

			goToFloor(nextFloor);
			
			while(getFloor() != nextFloor){
			//addNewOrder
				for(j=0;j<N_FLOORS;j++){
					if(direction == DIRN_UP){
						if(getButtonSignal(j,BUTTON_COMMAND)){
							printf("Button esel1\n");
							if(j> nextFloor){
								//Legge nextFloor på queue ok
								localQueue[nextFloor] = 1;
								nextFloor = j;
								goToFloor(nextFloor);

							}else if(j < nextFloor){
								//Legg i queue ok
								localQueue[j] = 1;
							}
						}
					}else{
						if(getButtonSignal(j,BUTTON_COMMAND)){
							printf("Button esel2\n");
							if(j< nextFloor){
								//Legge nextFloor på queue ok
								localQueue[nextFloor] = 1;
								nextFloor = j;
								goToFloor(nextFloor);

							}else if(j > nextFloor){
								//Legg i queue ok
								localQueue[j] = 1;
							}
						}
					}

					if (getButtonSignal(j,buttonCall)){
						printf("Button esel3\n");
						if(getFloor() <j && j< nextFloor){
							//QUEUQquqe ok
							localQueue[j] = 1;
						}
						if(getFloor()> j && j> nextFloor){
							localQueue[j] = 1;
						}/*if(getFloor() == j){
							setMotorDirection(DIRN_STOP);
							setDoorOpenLamp(1);
							usleep(500000);
							setDoorOpenLamp(0);
							goToFloor(nextFloor);
						}*/
					}
				}
				if(localQueue[getFloor()]== 1){
					setMotorDirection(DIRN_STOP);
					printf("Stop esel\n");
					setDoorOpenLamp(1);
					k=0;
					while(k<10000){k++;}
					setDoorOpenLamp(0);
					localQueue[getFloor()] == 0;
					goToFloor(nextFloor);
				}
				//printf("Sjekker etasje\n");
			}
			//int nextFloor = ..//getNeworder
			
			
			queueActive = 0;
			setMotorDirection(DIRN_STOP);
		}
	}
	setMotorDirection(DIRN_STOP);
	if(isObstructed()){
		printf("Elevator was obstructed\n");
	}
	if(isStopped()){
		setStopLamp(1);
		printf("Elevator was stopped\n");
	}
	if(check >= maxCheck){
		printf("Checked getFloor() %d times without sucess, and therefore stopped\n",check);
	}

	
	return 0;
}
