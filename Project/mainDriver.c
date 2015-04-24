
#include "mainDriver.h"
#include "orderManager.h"
#include "elevDriver.h"
#include "network_modulev2.h"
#include "costFunction.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#define N_FLOORS 4
void* mainDriver() {
	
	printf("Press STOP button to stop elevator and exit program.\n");
	//struct timespec testingtime, rem;
	int nextFloor = -1;
	int i, j,k;
	int lastFloor = 0;
	int currentFloor, tmp;
	int thisElevator = getLocalIP();
	time_t startTime, endTime;
	//double diffTime;
	//motorDirection direction;
	buttonType buttonCall;
	int newFloor,floorSetCommand,floorSetDown,floorSetUp,floorSetCommandRunning,floorSetDownRunning,floorSetUpRunning;
	int localQueue[N_FLOORS];
	time(&startTime);
	reportElevState(getFloor(), nextFloor, BUTTON_COMMAND);
	while(!isStopped() && !isObstructed()){
		floorSetCommand = -1;
		floorSetDown = -1;
		floorSetUp = -1;
		currentFloor = getFloor();
		setMotorDirection(DIRN_STOP);
		if (currentFloor != -1){
			setFloorIndicator(getFloor());
		}else{
			setFloorIndicator(lastFloor);
		}

		//setDoorOpenLamp(1);
		//nextFloor = -1;

		memset(localQueue,0,sizeof(int)*N_FLOORS);
		
		for (i = 0; i < N_FLOORS; i++){
			if(getButtonSignal(i,BUTTON_COMMAND) && floorSetCommand != i){
				if(i != currentFloor){
					struct order newOrder = {.dest = i, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
					newFloor = addNewOrder(newOrder,0,0);
					floorSetCommand = i;
					//while(getButtonSignal(i,BUTTON_COMMAND)){}
				}else{
					setDoorOpenLamp(1);
					for (k = 0; k < 100000; k++){
						setMotorDirection(DIRN_STOP);
					}
					setDoorOpenLamp(0);
				}
			}
			if(i< N_FLOORS-1){
				if (getButtonSignal(i,BUTTON_CALL_UP) && floorSetUp != i){
					if(i != currentFloor){
						struct order newOrder = {.dest = i, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,0,0);
						floorSetUp = i;
						//while(getButtonSignal(i,BUTTON_CALL_UP)){}							
														
					}else{
						setDoorOpenLamp(1);
						for (k = 0; k < 100000; k++){
							setMotorDirection(DIRN_STOP);
						}
						setDoorOpenLamp(0);
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
					}else{
						setDoorOpenLamp(1);
						for (k = 0; k < 100000; k++){
							setMotorDirection(DIRN_STOP);
						}
						setDoorOpenLamp(0);
					}
				}
			}
		}
		time(&endTime);
		if (difftime(endTime, startTime) > 2.0){
			reportElevState(currentFloor, nextFloor, BUTTON_COMMAND);
			time(&startTime);
		}
		nextFloor = getNewOrder(currentFloor, nextFloor, BUTTON_COMMAND);

		//printf("nextFloor %d\n",nextFloor );
		if(nextFloor != -1){
			
			//printf("Kommer ikke hit\n");
			localQueue[nextFloor] = 1;
			lastFloor = getFloor();
			if(nextFloor-getFloor()> 0){
				buttonCall = BUTTON_CALL_UP;
			}else{			
				buttonCall = BUTTON_CALL_DOWN;
			}
			printf("Report 0\n");
			reportElevState(currentFloor, nextFloor, buttonCall);
			setDoorOpenLamp(0);
			goToFloor(nextFloor);
			floorSetCommandRunning = -1;
			floorSetUpRunning = -1;
			floorSetDownRunning = -1;
			while((lastFloor != nextFloor) && (!isStopped() && !isObstructed())){
				tmp = getFloor();
				currentFloor = tmp;
				//printf("LOOP\n");
				if(tmp != -1 && tmp != lastFloor){
					printf("Last lastFloor equals ------------------------------------ %d\n",lastFloor);
					lastFloor = tmp;
					printf("Report 1\n");
					reportElevState(lastFloor, nextFloor, buttonCall);
					setFloorIndicator(lastFloor);

					printf("New lastFloor equals -------------------------------------- %d\n",lastFloor);

				}
				for(j=0;j<N_FLOORS;j++){
					
					if(getButtonSignal(j,BUTTON_COMMAND) && floorSetCommandRunning != j){
						printf("Add order command\n");
						struct order newOrder = {.dest = j, .buttonType = BUTTON_COMMAND, .elevator = thisElevator};
						newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
						//while(getButtonSignal(j,BUTTON_COMMAND)){}
						floorSetCommandRunning = j;
						printf("Leave add order\n");
						
					}
					if(j>0){
						if (getButtonSignal(j,BUTTON_CALL_DOWN) && floorSetDownRunning != j){
							printf("Add order down\n");
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_DOWN, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							//while(getButtonSignal(j,BUTTON_CALL_DOWN)){}
							floorSetDownRunning = j;
							
						}
					}
					if(j<N_FLOORS-1){
						if (getButtonSignal(j,BUTTON_CALL_UP) && floorSetUpRunning != j){
							printf("Add order up\n");
							struct order newOrder = {.dest = j, .buttonType = BUTTON_CALL_UP, .elevator = thisElevator};
							newFloor = addNewOrder(newOrder,lastFloor,nextFloor);
							//while(getButtonSignal(j,BUTTON_CALL_UP)){}
							floorSetUpRunning = j;
							
						}
					}
				}
				newFloor = getNewOrder(lastFloor,nextFloor, buttonCall);
				//printf("After get\n");
				if(newFloor != nextFloor && newFloor != -1){
					localQueue[newFloor] = 1;
					printf("localQueue___: %d %d %d %d\n", localQueue[0], localQueue[1], localQueue[2], localQueue[3]);
					if(buttonCall == BUTTON_CALL_UP && newFloor>nextFloor){
						nextFloor = newFloor;
						printf("Report 2\n");
						reportElevState(lastFloor, nextFloor, buttonCall);
					}else if(buttonCall == BUTTON_CALL_DOWN && newFloor < nextFloor){
						nextFloor = newFloor;
						printf("Report 3\n");
						reportElevState(lastFloor, nextFloor, buttonCall);
					}
					//usleep(800000);
					//printf("Enter new != next\n");
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
					newFloor = getNewOrder(lastFloor,nextFloor, buttonCall);
					if ((newFloor != -1) && (newFloor != nextFloor) && ((currentFloor != 0)||(currentFloor != N_FLOORS-1))){
						if (((newFloor < nextFloor) && (buttonCall == BUTTON_CALL_DOWN)) || ((newFloor > nextFloor) && (buttonCall == BUTTON_CALL_UP))){
							nextFloor = newFloor;
							goToFloor(nextFloor);
							localQueue[nextFloor] = 1;
						}
					}
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
			localQueue[lastFloor] = 0; 
			printf("lastFloor: %d, currentFloor: %d\n", lastFloor, currentFloor);
			nextFloor = -1;
			printf("Report 4\n");
			reportElevState(lastFloor, nextFloor, BUTTON_COMMAND);
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
			//testingtime.tv_sec = 2;
			//testingtime.tv_nsec = 0;
			//nanosleep(&testingtime, &rem);
		}
	}
	if(isObstructed()){
		printf("Elevator was obstructed\n");
	}
	if(isStopped()){
		setStopLamp(1);
		printf("Elevator was stopped\n");
	}
	return NULL;

}

/*
void* printFunction(){
	int i,j;
	while(1){
		for (i = 0; i < N_FLOORS; i++){
			if (getButtonSignal(i,BUTTON_COMMAND) || getButtonSignal(i,BUTTON_CALL_DOWN) || getButtonSignal(i,BUTTON_CALL_UP)){
				printf("------------------------------------------------------------\n");
				printf("getFloor() = %d, floor: %d, up: %d, down: %d, command: %d\n", getFloor(), i, getButtonSignal(i, BUTTON_CALL_UP), getButtonSignal(i, BUTTON_CALL_DOWN), getButtonSignal(i, BUTTON_COMMAND));
				printf("|Floor| BUTTON_COMMAND | BUTTON_CALL_UP | BUTTON_CALL_DOWN |\n");
				printf("------------------------------------------------------------\n");

				for (j= 0; j < N_FLOORS; j++){
					printf("|  %d  |        %d       |        %d       |         %d        |\n",j, getButtonLamp(j,BUTTON_COMMAND),getButtonLamp(j,BUTTON_CALL_UP),getButtonLamp(j,BUTTON_CALL_DOWN));
				}
			}
		}
	}
}*/


