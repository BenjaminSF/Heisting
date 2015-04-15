

#include "elevDriver.h"
#include "costFunction.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define N_FLOORS 4
/*
struct order{
	int dest;
	int buttonType;
	int elevator
};
struct{
	struct order Queue[100];
	int inUse[100];
	int costOfQueue[100];
	int localPri[100];
	pthread_mutex_t rwLock;
}orderQueue;
*/

int main() {

// Initialize hardware

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
	motorDirection direction;
	buttonType buttonCall;




	while(!isStopped() && !isObstructed()){
		setMotorDirection(DIRN_STOP);
		setFloorIndicator(getFloor());
		setDoorOpenLamp(1);
		
		int localQueue[N_FLOORS];
		memset(localQueue,0,sizeof(int)*N_FLOORS);
		currentFloor = getFloor();
		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND)){
				setButtonLamp(i, BUTTON_COMMAND, 1);
				if(i != currentFloor){
					struct order newOrder;
					newOrder.dest = i;
					newOrder.buttonType = BUTTON_COMMAND;
					newOrder.elevator = 1;
					addNewOrder(newOrder);
			}
			}
			if(i< N_FLOORS){
				if (getButtonSignal(i,BUTTON_CALL_UP)){
					setButtonLamp(i,BUTTON_CALL_UP,1);
					if(i != currentFloor){
						struct order newOrder;
						newOrder.dest = i;
						newOrder.buttonType = BUTTON_CALL_UP;
						newOrder.elevator = 0;
						addNewOrder(newOrder);							//Fullfører driver for en heis
														//Men skal i costfunction ved flere heiser
					}
				}
			}
			if(i>0){
				if (getButtonSignal(i,BUTTON_CALL_DOWN)){
					setButtonLamp(i,BUTTON_CALL_DOWN,1);
					if (i != currentFloor){
						struct order newOrder;
						newOrder.dest = i;
						newOrder.buttonType = BUTTON_CALL_DOWN;
						newOrder.elevator = 0;
						addNewOrder(newOrder);
					}
				}
			}
		}
		nextFloor = getNewOrder(currentFloor);
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
					if(direction == DIRN_UP){
						if(getButtonSignal(j,BUTTON_COMMAND)){
							//printf("Got button signal upwards %d \n",j);
							setButtonLamp(j,BUTTON_COMMAND,1);
							if(j> nextFloor){
								localQueue[nextFloor] = 1;
								nextFloor = j;
							}else if(lastFloor <j < nextFloor){
								localQueue[j] = 1;
							}else{
								struct order newOrder;
								newOrder.dest = j;
								newOrder.buttonType = BUTTON_COMMAND;
								newOrder.elevator = 1;
							}
						}
					}else{
						if(getButtonSignal(j,BUTTON_COMMAND)){
							setButtonLamp(j,BUTTON_COMMAND,1);
							if(j< nextFloor){
								localQueue[nextFloor] = 1;
								nextFloor = j;
							}else if(lastFloor >j > nextFloor){
								localQueue[j] = 1;
							}else{
								struct order newOrder;
								newOrder.dest = j;
								newOrder.buttonType = BUTTON_COMMAND;
								newOrder.elevator = 1;
							}
						}
					}

					if (getButtonSignal(j,buttonCall)){
						//printf("Got a button call for %d\n",j);
						if(lastFloor <j && j< nextFloor){
							setButtonLamp(j,BUTTON_CALL_UP,1);
							localQueue[j] = 1;
							//printf("Will stop at %d on the way up\n",j);
						}
						if(lastFloor> j && j> nextFloor){
							setButtonLamp(j,BUTTON_CALL_DOWN,1);
							localQueue[j] = 1;
							//printf("Will stop at %d on the way down\n",j);

						
					}
				}
			}
				if(localQueue[getFloor()]== 1 && (getFloor() != -1)){
					setMotorDirection(DIRN_STOP);
					//printf("Stopping at floor %d\n",getFloor());
					setDoorOpenLamp(1);
					setFloorIndicator(getFloor());
					setButtonLamp(getFloor(),BUTTON_COMMAND,0);
					setButtonLamp(getFloor(),BUTTON_CALL_DOWN,0);
					setButtonLamp(getFloor(),BUTTON_CALL_UP,0);

					k=0;
					//printf("Waiting\n");
					while((k<100000) && (!isStopped() && !isObstructed())){
						k++;
						setMotorDirection(DIRN_STOP);
					}
					setDoorOpenLamp(0);
					localQueue[getFloor()] = 0;
					//printf("Starting again\n");
					goToFloor(nextFloor);
				}			
		}
	}
	queueActive = 0;
	for (i = 0; i < N_FLOORS; i++){
		setButtonLamp(i,BUTTON_COMMAND,0);
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