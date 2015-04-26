#include "costFunction.h"
#include "publicTypes.h"
#include <stdlib.h>

int findCost(int orderFloor,int elevFloor, int elevNextFloor,int orderButton, int elevButton){
	int costDir = elevNextFloor - elevFloor;
	int cost = orderFloor - elevFloor;
	
	if ((elevButton == BUTTON_CALL_UP) && ((orderButton == BUTTON_CALL_DOWN) || (orderFloor < elevFloor))){
		cost = (N_FLOORS - elevFloor) + (N_FLOORS - orderFloor) - 2 + N_FLOORS;
	}else if ((elevButton == BUTTON_CALL_DOWN) && ((orderButton == BUTTON_CALL_UP) || (orderFloor > elevFloor))){
		cost = orderFloor + elevFloor + N_FLOORS;
	}else if (elevNextFloor == -1){
		cost = abs(cost);
	}else if ((orderButton == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_UP) && (elevFloor > orderFloor)){
		cost = ((N_FLOORS - elevFloor) + (N_FLOORS - orderFloor)) - 2 + N_FLOORS;
	}else if ((orderButton == BUTTON_COMMAND) && (elevButton == BUTTON_CALL_DOWN) && (elevFloor < orderFloor)){
		cost = (orderFloor + elevFloor) + N_FLOORS;
	}else if (orderButton == BUTTON_COMMAND){
		cost = abs(cost);
	}else if (elevButton == orderButton){
		cost = abs(cost);
	}else if ((cost * costDir) >= 0 && ((costDir>0 && (orderButton == 0 || orderButton == 2))||(costDir<0 && (orderButton == 2 || orderButton == 1)))){
		cost = abs(cost);
	}else{
		cost = 2*N_FLOORS + 1;
	}
	return cost;
}
