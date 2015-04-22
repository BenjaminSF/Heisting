
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
	
	int nextFloor;
	int i, j,k;
	int queueActive = 0;
	int lastFloor = 0;
	int currentFloor;
	int thisElevator = getLocalIP();
	motorDirection direction;
	buttonType buttonCall;
	int newFloor,floorSetCommand,floorSetDown,floorSetUp;



	while(!isStopped() && !isObstructed()){
		floorSetCommand = -1;
		floorSetDown = -1;
		floorSetUp = -1;
		setMotorDirection(DIRN_STOP);
		setFloorIndicator(getFloor());
		setDoorOpenLamp(1);
		//nextFloor = -1;
		int localQueue[N_FLOORS];
		buttonType localQueueButtonType[N_FLOORS];
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
			while((getFloor() != nextFloor) && (!isStopped() && !isObstructed())){
				for(j=0;j<N_FLOORS;j++){
					
					if(getButtonSignal(j,BUTTON_COMMAND)){
						struct order newOrder = {.dest = j, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
						while(getButtonSignal(j,BUTTON_COMMAND)){}
						if(newFloor != -1){
							localQueue[newFloor] = 1;
							localQueueButtonType[newFloor] = newOrder.buttonType;
						}
					}
					if(j>0){
						if (getButtonSignal(j,BUTTON_CALL_DOWN)){
							
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							while(getButtonSignal(j,BUTTON_CALL_DOWN)){}
							if(newFloor != -1){
								localQueue[newFloor] = 1;
								localQueueButtonType[newFloor] = newOrder.buttonType;
							}
						}
					}
					if(j<N_FLOORS-1){
						if (getButtonSignal(j,BUTTON_CALL_UP)){
														
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							while(getButtonSignal(j,BUTTON_CALL_UP)){}
							if(newFloor != -1){
								localQueue[newFloor] = 1;
								localQueueButtonType[newFloor] = newOrder.buttonType;	
							}	
						}
						}
					}
				if(localQueue[getFloor()]== 1 && (getFloor() != -1)){
					setMotorDirection(DIRN_STOP);
					deleteOrder(getFloor(),localQueueButtonType[getFloor()], thisElevator);
					setDoorOpenLamp(1);
					setFloorIndicator(getFloor());
					setButtonLamp(getFloor(),localQueueButtonType[getFloor()],0);
					k=0;
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
					setDoorOpenLamp(0);
					localQueue[getFloor()] = 0;
					goToFloor(nextFloor);
				}			
				if (getFloor() == nextFloor){
					k = 0;
					deleteOrder(getFloor(), localQueueButtonType[getFloor()], thisElevator);
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
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
