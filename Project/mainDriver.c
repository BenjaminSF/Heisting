
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
		
		printf("nextFloor %d\n",nextFloor );
		if(nextFloor != -1){
			printf("Kommer ikke hit\n");
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
			while((getFloor() != nextFloor) && (!isStopped() && !isObstructed())){
				printf("Top while\n");
				for(j=0;j<N_FLOORS;j++){
					
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
				printf("After for, in while\n");
				newFloor = getNewOrder(lastFloor,nextFloor);
				printf("After getNewOrder\n");
				if(newFloor != nextFloor){
					printf("Enter new != next\n");
					localQueue[newFloor] = 1;
				}
				if(localQueue[getFloor()]== 1 && (getFloor() != -1)){
					printf("Enter localQueue\n");
					setMotorDirection(DIRN_STOP);
					deleteOrder(getFloor(),buttonCall, thisElevator);
					deleteOrder(getFloor(),BUTTON_COMMAND,thisElevator);
					setDoorOpenLamp(1);
					setFloorIndicator(getFloor());
					setButtonLamp(getFloor(),buttonCall,0);
					setButtonLamp(getFloor(),BUTTON_COMMAND,0);
					k=0;
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
					setDoorOpenLamp(0);
					localQueue[getFloor()] = 0;
					goToFloor(nextFloor);
				}	
				printf("getFloor: %d\n", getFloor());
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
				if(getFloor() != -1){
					lastFloor = getFloor();
				}
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


void* printFunction(){
	int i,j;
	while(1){
		for (i = 0; i < N_FLOORS; i++){
			if (getButtonSignal(i,BUTTON_COMMAND) || getButtonSignal(i,BUTTON_CALL_DOWN) || getButtonSignal(i,BUTTON_CALL_UP)){
				printf('-----------------------------------------\n')
				printf('getFloor() = %d\n'getFloor())
				printf('|Floor| BUTTON_COMMAND | BUTTON_CALL_UP | BUTTON_CALL_DOWN|\n')
				for (j= 0; i < N_FLOORS; i++){
					printf('|  %d  |  %d  |  %d  |  %d  |\n',getButtonLamp(j,BUTTON_COMMAND),getButtonLamp(j,BUTTON_CALL_UP),getButtonLamp(j,BUTTON_CALL_DOWN));
				}
			}
		}
	}
}


