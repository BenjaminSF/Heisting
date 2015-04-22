
#include "mainDriver.h"
#include "elevDriver.h"
#include "network_modulev2.h"
#include "costFunction.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define N_FLOORS 4
void* mainDriver() {
	if (!elevDriver_initialize()) {
		printf("Unable to initialize elevator hardware!\n");
		return;
}
	printf("Press STOP button to stop elevator and exit program.\n");
	
	int nextFloor = -1;
	int i, j,k;
	int queueActive = 0;
	int lastFloor = 0;
	int currentFloor;
	int thisElevator = getLocalIP();
	motorDirection direction;
	buttonType buttonCall;
	int newFloor,floorSetCommand,floorSetDown,floorSetUp,floorSetCommandRunning,floorSetDownRunning,floorSetUpRunning;
	int localQueue[N_FLOORS];
	while(!isStopped() && !isObstructed()){
		floorSetCommand = -1;
		floorSetDown = -1;
		floorSetUp = -1;
		setMotorDirection(DIRN_STOP);
		setFloorIndicator(getFloor());
		setDoorOpenLamp(1);
		//nextFloor = -1;

		memset(localQueue,0,sizeof(int)*N_FLOORS);
		currentFloor = getFloor();
		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND) && floorSetCommand != i){
				if(i != currentFloor){
					struct order newOrder = {.dest = i, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
					newFloor = addNewOrder(newOrder,0,0);
					floorSetCommand = i;
					//while(getButtonSignal(i,BUTTON_COMMAND)){}
			}
			}
			if(i< N_FLOORS-1){
				if (getButtonSignal(i,BUTTON_CALL_UP) && floorSetUp != i){
					if(i != currentFloor){
						struct order newOrder = {.dest = i, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,0,0);
						floorSetUp = i;
						//while(getButtonSignal(i,BUTTON_CALL_UP)){}							
														
					}
				}
			}
			if(i>0){
				if (getButtonSignal(i,BUTTON_CALL_DOWN) && floorSetDown != i){
					if (i != currentFloor){
						struct order newOrder = {.dest = i, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,0,0);
						floorSetDown = i;
						//while(getButtonSignal(i,BUTTON_CALL_DOWN)){}
					}
				}
			}
		}
		nextFloor = getNewOrder(currentFloor, nextFloor);
		
		//printf("nextFloor %d\n",nextFloor );
		if(nextFloor != -1){
			//printf("Kommer ikke hit\n");
			localQueue[nextFloor] = 1;
			lastFloor = getFloor();
			if(nextFloor-getFloor()> 0){
				direction = DIRN_UP;
				buttonCall = BUTTON_CALL_UP;
			}else{
				direction = DIRN_DOWN;
				buttonCall = BUTTON_CALL_DOWN;
			}
			setDoorOpenLamp(0);
			goToFloor(nextFloor);
			floorSetCommandRunning = -1;
			floorSetUpRunning = -1;
			floorSetDownRunning = -1;
			while((lastFloor != nextFloor) && (!isStopped() && !isObstructed())){
				for(j=0;j<N_FLOORS;j++){
					int tmp = getFloor();
					currentFloor = tmp;
					if(tmp != -1 && tmp != lastFloor){
						printf("Last lastFloor equals ------------------------------------ %d\n",lastFloor);
						lastFloor = tmp;
						setFloorIndicator(lastFloor);

						printf("New lastFloor equals -------------------------------------- %d\n",lastFloor);

					}
					if(getButtonSignal(j,BUTTON_COMMAND) && floorSetCommandRunning != j){
						struct order newOrder = {.dest = j, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
						//while(getButtonSignal(j,BUTTON_COMMAND)){}
						floorSetCommandRunning = j;
						
					}
					if(j>0){
						if (getButtonSignal(j,BUTTON_CALL_DOWN) && floorSetDownRunning != j){
							
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							//while(getButtonSignal(j,BUTTON_CALL_DOWN)){}
							floorSetDownRunning = j;
							
						}
					}
					if(j<N_FLOORS-1){
						if (getButtonSignal(j,BUTTON_CALL_UP) && floorSetUpRunning != j){
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							//while(getButtonSignal(j,BUTTON_CALL_UP)){}
							floorSetUpRunning = j;
							
						}
					}
				}
				newFloor = getNewOrder(lastFloor,nextFloor);
				if(newFloor != nextFloor && newFloor != -1){
					localQueue[newFloor] = 1;
					if(buttonCall == BUTTON_CALL_UP && newFloor>nextFloor){
						nextFloor = newFloor;
					}else if(buttonCall == BUTTON_CALL_DOWN && newFloor < nextFloor){
						nextFloor = newFloor;
					}

					printf("Enter new != next\n");
				}
				if((localQueue[lastFloor]== 1) && (currentFloor == lastFloor)){
					printf("Enter localQueue\n");
					setMotorDirection(DIRN_STOP);
					deleteOrder(lastFloor,buttonCall, thisElevator);
					deleteOrder(lastFloor,BUTTON_COMMAND,thisElevator);
					setDoorOpenLamp(1);
					if ((buttonCall == BUTTON_CALL_DOWN && lastFloor != 0) || (buttonCall==BUTTON_CALL_UP && lastFloor != (N_FLOORS-1))){
						setButtonLamp(lastFloor,buttonCall,0);
					}else if(buttonCall == BUTTON_CALL_DOWN && lastFloor == 0){
						setButtonLamp(lastFloor,BUTTON_CALL_UP,0);
					}else if(buttonCall == BUTTON_CALL_UP && lastFloor == N_FLOORS-1){
						setButtonLamp(lastFloor,BUTTON_CALL_DOWN,0);
					}
					setButtonLamp(lastFloor,BUTTON_COMMAND,0);
					k=0;
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
					setDoorOpenLamp(0);
					localQueue[lastFloor] = 0;
					goToFloor(nextFloor);
				}	
				//printf("getFloor: %d\n", lastFloor);
				/*if (getFloor() == nextFloor){
					k = 0;
					printf("Dørene åpnes\n");
					deleteOrder(getFloor(), buttonCall, thisElevator);
					deleteOrder(getFloor(),BUTTON_COMMAND,thisElevator);
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
				}*/
				
			}
			nextFloor = -1;
			k = 0;
			printf("Dørene åpnes\n");
			deleteOrder(getFloor(), BUTTON_CALL_UP, thisElevator);
			deleteOrder(getFloor(), BUTTON_CALL_DOWN, thisElevator);
			deleteOrder(getFloor(),BUTTON_COMMAND,thisElevator);
			while((k<100000) && (!isStopped() && !isObstructed())){
				k++;
				setMotorDirection(DIRN_STOP);
			}
			printf("localQueue: %d %d %d %d\n", localQueue[0], localQueue[1], localQueue[2], localQueue[3]);
			for(k = 0; k < N_FLOORS; k++){
				if (localQueue[k] != 0){
					printf("ERROR!!!! Sjekk localQueue____________________");
				}
			}
		}
	}
	if(isObstructed()){
		printf("Elevator was obstructed\n");
	}
	if(isStopped()){
		setStopLamp(1);
		printf("Elevator was stopped\n");
	}
	return;

}
