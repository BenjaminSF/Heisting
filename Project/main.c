

#include "elevDriver.h"
#include "costFunction.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define N_FLOORS 4
int main() {
	if (!elevDriver_initialize()) {
		printf("Unable to initialize elevator hardware!\n");
		return 1;
}
	printf("Press STOP button to stop elevator and exit program.\n");
	
	int nextFloor = 0;
	int i, j,k;
	int queueActive = 0;
	int lastFloor = 0;
	int currentFloor;
	int thisElevator = 1;
	motorDirection direction;
	buttonType buttonCall;




	while(!isStopped() && !isObstructed()){
		setMotorDirection(DIRN_STOP);
		setFloorIndicator(getFloor());
		setDoorOpenLamp(1);
		nextFloor = -1;
		int localQueue[N_FLOORS];
		buttonType localQueueButtonType[N_FLOORS];
		memset(localQueue,0,sizeof(int)*N_FLOORS);
		currentFloor = getFloor();
		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND)){
				if(i != currentFloor){
					struct order newOrder;
					newOrder.dest = i;
					newOrder.buttonType = BUTTON_COMMAND;
					newOrder.elevator = thisElevator;
					addNewOrder(newOrder);
					while(getButtonSignal(i,BUTTON_COMMAND)){}
			}
			}
			if(i< N_FLOORS-1){
				if (getButtonSignal(i,BUTTON_CALL_UP)){
					if(i != currentFloor){
						struct order newOrder;
						newOrder.dest = i;
						newOrder.buttonType = BUTTON_CALL_UP;
						newOrder.elevator = thisElevator;
						addNewOrder(newOrder);
						while(getButtonSignal(i,BUTTON_CALL_UP)){}							
														
					}
				}
			}
			if(i>0){
				if (getButtonSignal(i,BUTTON_CALL_DOWN)){
					if (i != currentFloor){
						struct order newOrder;
						newOrder.dest = i;
						newOrder.buttonType = BUTTON_CALL_DOWN;
						newOrder.elevator = thisElevator;
						addNewOrder(newOrder);
						while(getButtonSignal(i,BUTTON_CALL_DOWN)){}
					}
				}
			}
		}
		nextFloor = getNewOrder(currentFloor);
		
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
						struct order newOrder;
						newOrder.dest = j;
						newOrder.buttonType = BUTTON_COMMAND;
						newOrder.elevator = thisElevator;
						newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
						while(getButtonSignal(j,BUTTON_COMMAND)){}
						if(newFloor != -1){
							localQueue[newFloor] = 1;
							localQueueButtonType[newFloor] = newOrder.buttonType;
						}
					}
					if(j>0){
						if (getButtonSignal(j,BUTTON_CALL_DOWN)){
							
							struct order newOrder;
							newOrder.dest = j;
							newOrder.buttonType = BUTTON_CALL_DOWN;
							newOrder.elevator = thisElevator;
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
														
							struct order newOrder;
							newOrder.dest = j;
							newOrder.buttonType = BUTTON_CALL_UP;
							newOrder.elevator = thisElevator;
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
					//printf("Stopping at floor %d\n",getFloor());
					setDoorOpenLamp(1);
					setFloorIndicator(getFloor());
					//setButtonLamp(getFloor(),BUTTON_COMMAND,0);
					//setButtonLamp(getFloor(),BUTTON_CALL_DOWN,0);
					//setButtonLamp(getFloor(),BUTTON_CALL_UP,0);

					k=0;
					//printf("Waiting\n");
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
	return 0;

}
