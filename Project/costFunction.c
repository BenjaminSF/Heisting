#include "costFunction.h"
#include "publicTypes.h"
#include <stdlib.h>

int findCost(int costFloor,int currentFloor, int nextFloor,int buttonType, int elevButton, int direction){
	int cost;
	int costDir = nextFloor - currentFloor;
	cost = costFloor - currentFloor;
	
	if ((elevButton == BUTTON_CALL_UP) && ((buttonType == BUTTON_CALL_DOWN) || (costFloor < currentFloor))){
		//cost = 2*N_FLOORS;
		cost = (N_FLOORS - currentFloor) + (N_FLOORS - costFloor) - 2 + N_FLOORS;
	}else if ((elevButton == BUTTON_CALL_DOWN) && ((buttonType == BUTTON_CALL_UP) || (costFloor > currentFloor))){
		//cost = 2*N_FLOORS;
		cost = costFloor + currentFloor + N_FLOORS;
	}else if (nextFloor == -1){
		cost = abs(cost);
	}else if ((buttonType == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_UP) && (currentFloor > costFloor)){
		//cost = 2*N_FLOORS;
		cost = ((N_FLOORS - currentFloor) + (N_FLOORS - costFloor)) - 2 + N_FLOORS;
	}else if ((buttonType == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_DOWN) && (currentFloor < costFloor)){
		//cost = 2*N_FLOORS;
		cost = (costFloor + currentFloor) + N_FLOORS;
	}else if (buttonType == BUTTON_COMMAND){
		cost = abs(cost);
	}else if (elevButton == buttonType){
		cost = abs(cost);
	}else if ((cost * costDir) >= 0 && ((costDir>0 && (buttonType == 0 || buttonType == 2))||(costDir<0 && (buttonType == 2 || buttonType == 1)))){
		cost = abs(cost);
		printf("nextFloor right way\n");
	}else{
		printf("This should not happen!!!!!!!!!!!!!\n");
		cost = 2*N_FLOORS + 1;
	}
	if (direction * costDir < 0){
		cost += 2;
	}
	//printf("findCost: current: %d, next: %d, cost: %d\n", currentFloor, nextFloor, cost);
	return cost;
}

/*int findCost(int costFloor, int currentFloor, int nextFloor, int buttonType, int elevButton){
	int cost;
	cost = 2* abs(costFloor - currentFloor);
	if (nextFloor > currentFloor && elevButton == BUTTON_CALL_DOWN){
		cost += costFloor + currentFloor;
	}else if (nextFloor < currentFloor && elevButton == BUTTON_CALL_UP){
		cost += 2* N_FLOORS - currentFloor - nextFloor -2;
	}
	if (buttonType == BUTTON_CALL_DOWN && costFloor > currentFloor){
		cost++;
	}else if(buttonType == BUTTON_CALL_UP && costFloor < currentFloor){
		cost++;
	}	
	if (buttonType != BUTTON_COMMAND){
		cost++;
	}
	return cost;
}*/