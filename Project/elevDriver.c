#include "channels.h"
#include "io.h"
#include "elevDriver.h"
#include "network_modulev2.h"
#include "costFunction.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define N_BUTTONS 3

static const int lampMatrix[N_FLOORS][N_BUTTONS] = {
	{LIGHT_UP1, LIGHT_DOWN1, LIGHT_COMMAND1},
	{LIGHT_UP2, LIGHT_DOWN2, LIGHT_COMMAND2},
	{LIGHT_UP3, LIGHT_DOWN3, LIGHT_COMMAND3},
	{LIGHT_UP4, LIGHT_DOWN4, LIGHT_COMMAND4},
};
static const int buttonMatrix[N_FLOORS][N_BUTTONS] = {
	{BUTTON_UP1, BUTTON_DOWN1, BUTTON_COMMAND1},
	{BUTTON_UP2, BUTTON_DOWN2, BUTTON_COMMAND2},
	{BUTTON_UP3, BUTTON_DOWN3, BUTTON_COMMAND3},
	{BUTTON_UP4, BUTTON_DOWN4, BUTTON_COMMAND4},
};

int elevDriver_initialize(void) {
	int i;

	// Init hardware
	if (!io_init())
		return 0;
	// Zero all floor button lamps
	for (i = 0; i < N_FLOORS; ++i) {
		if (i != 0)
			setButtonLamp(i, BUTTON_CALL_DOWN, 0);
		if (i != N_FLOORS - 1)
			setButtonLamp(i, BUTTON_CALL_UP, 0);
			setButtonLamp(i, BUTTON_COMMAND, 0);
	}
	// Clear stop lamp, door open lamp, and set floor indicator to current floor.
	setStopLamp(0);
	setDoorOpenLamp(0);

	if(getFloor() == -1){
		setMotorDirection(DIRN_DOWN);
		int k = 0;
		while(getFloor() == -1){
			usleep(1000);
			k++;
			if (k> 10000){
				setMotorDirection(DIRN_UP);
				k =0;
			}
		}
	}
	setFloorIndicator(getFloor());
	setMotorDirection(DIRN_STOP);
	initPriorityQueue();

	return 1;
}
void setMotorDirection(motorDirection direction) {
	if (direction < 0){
		io_set_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
}	else if (direction > 0) {
		io_clear_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
} 	else{
		io_write_analog(MOTOR, 0);
}
}
motorDirection getMotorDirection(void){
	motorDirection direction = io_read_bit(MOTORDIR);
	return direction;
}

void goToFloor(int floor){
	motorDirection direction = DIRN_STOP;
	if(getFloor() == -1){
		while(getFloor() == -1){
			//wait
		}
	}
	int currentFloor = getFloor(),diff = floor-currentFloor;

	assert(floor>= 0 && floor < N_FLOORS);
	if (diff > 0){
		direction = DIRN_UP;
	}else if(diff < 0){
		direction = DIRN_DOWN;
	}else{
		direction = DIRN_STOP;
	}
	setMotorDirection(direction);
}


void setDoorOpenLamp(int status){
	if (status){
		io_set_bit(LIGHT_DOOR_OPEN);
	}else{
		io_clear_bit(LIGHT_DOOR_OPEN);

	}
}

int isObstructed(void){
	return io_read_bit(OBSTRUCTION);
}

int isStopped(void){
	return io_read_bit(STOP);
}

void setStopLamp(int status){
	if (!(getFloor() == -1) && status){
		setDoorOpenLamp(1);
		printf("The elevator has stopped. Please leave the elevator.\n");
	}
	if (status){
		io_set_bit(LIGHT_STOP);
	}else{
		io_clear_bit(LIGHT_STOP);
	}
}

int getFloor(void){
	int sensorFloors[4] = {SENSOR_FLOOR1,SENSOR_FLOOR2,SENSOR_FLOOR3,SENSOR_FLOOR4};
	int i;
	for (i = 0; i < 4; ++i)
	{
		if (io_read_bit(sensorFloors[i]))
		{
			return i;
		}
	}
	return -1;
}

void setFloorIndicator(int floor){
	assert(floor >= 0);
	assert(floor < N_FLOORS);
	// Binary encoding. One light must always be on.
	if (floor & 0x02)
		io_set_bit(LIGHT_FLOOR_IND1);
	else
		io_clear_bit(LIGHT_FLOOR_IND1);
	if (floor & 0x01)
		io_set_bit(LIGHT_FLOOR_IND2);
	else
		io_clear_bit(LIGHT_FLOOR_IND2);
}
/*
int isbuttonSignalValid(int floor, buttonType button){
	//printf("Assert: Floor: %d, type: %d\n", floor, button);
	assert(floor>= 0 && floor < N_FLOORS);
	assert(!(button == BUTTON_CALL_UP && floor == N_FLOORS-1) && !(button == BUTTON_CALL_DOWN && floor == 0));
	assert(button == BUTTON_CALL_DOWN || button == BUTTON_CALL_UP || button == BUTTON_COMMAND);
	return 1;
}*/
int getButtonSignal(int floor, buttonType button){
	return io_read_bit(buttonMatrix[floor][button]);
}
void setButtonLamp(int floor, buttonType button, int status){
	//if (isbuttonSignalValid(floor,button)){
	if (status){
		io_set_bit(lampMatrix[floor][button]);
	}else{
		io_clear_bit(lampMatrix[floor][button]);
	}
	//}
}
int getButtonLamp(int floor,buttonType button){
	return io_read_bit(lampMatrix[floor][button]);
}


