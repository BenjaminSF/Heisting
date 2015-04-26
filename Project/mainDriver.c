#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "mainDriver.h"
#include "publicTypes.h"
#include "orderManager.h"
#include "elevDriver.h"

void* mainDriver(void *args) {
	int i, j, k, currentFloor, checkLocal, newFloor, floorSetCommand, floorSetDown, floorSetUp;
	time_t startTime, endTime;
	buttonType buttonCall;
	int localQueue[N_FLOORS] = {0};
	int lastFloor = 0;
	int nextFloor = -1;
	int thisElevator = *(int *) args;
	time(&startTime);
	reportElevState(getFloor(), nextFloor, BUTTON_COMMAND);
	struct timespec ts,rem;
	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	while(1){
		currentFloor = getFloor();
		setMotorDirection(DIRN_STOP);
		if (currentFloor != -1){
			setFloorIndicator(currentFloor);
		}else{
			setFloorIndicator(lastFloor);
		}
		for (i = 0; i < N_FLOORS; i++){ //Check button presses
			if(getButtonSignal(i,BUTTON_COMMAND)){
				if(i != currentFloor){
					Order newOrder = {.dest = i, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
					addNewOrder(newOrder);
				}else{
					setDoorOpenLamp(1);
					setMotorDirection(DIRN_STOP);
					nanosleep(&ts, &rem);
					setDoorOpenLamp(0);
				}
			}
			if(i< N_FLOORS-1){
				if (getButtonSignal(i,BUTTON_CALL_UP)){
					if(i != currentFloor){
						Order newOrder = {.dest = i, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
						addNewOrder(newOrder);								
					}else{
						setDoorOpenLamp(1);
						setMotorDirection(DIRN_STOP);
						nanosleep(&ts, &rem);
						setDoorOpenLamp(0);
					}
				}
			}
			if(i>0){
				if (getButtonSignal(i,BUTTON_CALL_DOWN)){
					if (i != currentFloor){
						Order newOrder = {.dest = i, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
						addNewOrder(newOrder);
					}else{
						setDoorOpenLamp(1);
						setMotorDirection(DIRN_STOP);
						nanosleep(&ts, &rem);
						setDoorOpenLamp(0);
					}
				}
			}
		}
		time(&endTime); //Update position every two seconds
		if (difftime(endTime, startTime) > 2.0){
			reportElevState(currentFloor, nextFloor, BUTTON_COMMAND);
			time(&startTime);
		}
		if (nextFloor == -1) nextFloor = getNewOrder(currentFloor, nextFloor, BUTTON_COMMAND);
		if(nextFloor != -1){
			localQueue[nextFloor] = 1;
			lastFloor = getFloor();
			if(nextFloor-getFloor()> 0){
				buttonCall = BUTTON_CALL_UP;
			}else{			
				buttonCall = BUTTON_CALL_DOWN;
			}
			reportElevState(currentFloor, nextFloor, buttonCall);
			if (nextFloor == lastFloor){
				setDoorOpenLamp(1);
				nanosleep(&ts, &rem);
			}
			setDoorOpenLamp(0);
			goToFloor(nextFloor);
			floorSetCommand = -1;
			floorSetUp = -1;
			floorSetDown = -1;
			while(lastFloor != nextFloor){
				currentFloor = getFloor();
				if(currentFloor != -1 && currentFloor != lastFloor){
					lastFloor = currentFloor;
					reportElevState(lastFloor, nextFloor, buttonCall);
					setFloorIndicator(lastFloor);
				}
				for(j=0;j<N_FLOORS;j++){
					if(getButtonSignal(j,BUTTON_COMMAND) && floorSetCommand != j){
						Order newOrder = {.dest = j, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
						addNewOrder(newOrder);
						floorSetCommand = j;
					}
					if(j>0){
						if (getButtonSignal(j,BUTTON_CALL_DOWN) && floorSetDown != j){
							Order newOrder = {.dest = j, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
							addNewOrder(newOrder);
							floorSetDown = j;
						}
					}
					if(j<N_FLOORS-1){
						if (getButtonSignal(j,BUTTON_CALL_UP) && floorSetUp != j){
							Order newOrder = {.dest = j, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
							addNewOrder(newOrder);
							floorSetUp = j;
						}
					}
				}
				newFloor = getNewOrder(lastFloor,nextFloor, buttonCall);
				if(newFloor != nextFloor && newFloor != -1){
					localQueue[newFloor] = 1;
					if(buttonCall == BUTTON_CALL_UP && newFloor>nextFloor){
						nextFloor = newFloor;
					}else if(buttonCall == BUTTON_CALL_DOWN && newFloor < nextFloor){
						nextFloor = newFloor;
					}
					reportElevState(lastFloor, nextFloor, buttonCall);
				}
				if((localQueue[lastFloor]== 1) && (currentFloor == lastFloor)){
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
					setMotorDirection(DIRN_STOP);
					nanosleep(&ts, &rem);
					setDoorOpenLamp(0);
					localQueue[lastFloor] = 0;
					goToFloor(nextFloor);
					newFloor = getNewOrder(lastFloor,nextFloor, buttonCall);
					if ((newFloor != -1) && (newFloor != nextFloor) && ((currentFloor != 0) && (currentFloor != N_FLOORS-1))){
						if (((newFloor < nextFloor) && (buttonCall == BUTTON_CALL_DOWN)) || ((newFloor > nextFloor) && (buttonCall == BUTTON_CALL_UP))){
							nextFloor = newFloor;
							goToFloor(nextFloor);
							localQueue[nextFloor] = 1;
							reportElevState(lastFloor, nextFloor, buttonCall);
						}
					}
				}			
			} 
			localQueue[lastFloor] = 0; 
			nextFloor = -1;
			reportElevState(lastFloor, nextFloor, BUTTON_COMMAND);
			deleteOrder(getFloor(), BUTTON_CALL_UP, thisElevator);
			deleteOrder(getFloor(), BUTTON_CALL_DOWN, thisElevator);
			deleteOrder(getFloor(),BUTTON_COMMAND,thisElevator);
			setMotorDirection(DIRN_STOP);
			nanosleep(&ts, &rem);
			checkLocal = N_FLOORS;
			for(k = 0; k < N_FLOORS; k++){
				if (localQueue[k] == 1){
					if (abs(lastFloor - k) < checkLocal) checkLocal = k;
				}
			}
			if (checkLocal < N_FLOORS) nextFloor = checkLocal;
		} 
	}
	return NULL;
}