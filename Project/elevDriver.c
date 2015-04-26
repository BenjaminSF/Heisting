#include "channels.h"
#include "io.h"
#include "elevDriver.h"
#include "orderManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


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
	if (!io_init())
		return 0;
	for (i = 0; i < N_FLOORS; ++i) {
		if (i != 0) setButtonLamp(i, BUTTON_CALL_DOWN, 0);
		if (i != N_FLOORS - 1)
			setButtonLamp(i, BUTTON_CALL_UP, 0);
			setButtonLamp(i, BUTTON_COMMAND, 0);
	}
	io_clear_bit(LIGHT_STOP);
	setDoorOpenLamp(0);
	if(getFloor() == -1){
		setMotorDirection(DIRN_DOWN);
		int k = 0;
		while(getFloor() == -1){
			usleep(1000);
			k++;
			if (k> 10000){
				setMotorDirection(DIRN_UP);
				k=0;
			}
		}
	}
	setFloorIndicator(getFloor());
	setMotorDirection(DIRN_STOP);
	initPriorityQueue();
	return 1;
}

void setMotorDirection(motorDirection direction){
	if (direction < 0){
		io_set_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
	}else if (direction > 0){
		io_clear_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
	}else{
		io_write_analog(MOTOR, 0);
	}
}

motorDirection getMotorDirection(void){
	motorDirection direction = io_read_bit(MOTORDIR);
	return direction;
}

void goToFloor(int floor){

	motorDirection direction = DIRN_STOP;
	while(getFloor() == -1){}
	int currentFloor = getFloor();
	int diff = floor-currentFloor;
	if(getFloor() != -1){
		if (diff > 0){
			direction = DIRN_UP;
		}else if(diff < 0){
			direction = DIRN_DOWN;
		}
		setMotorDirection(direction);
	}
}

void setDoorOpenLamp(int status){
	if (status){
		io_set_bit(LIGHT_DOOR_OPEN);
	}else{
		io_clear_bit(LIGHT_DOOR_OPEN);

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
	if (floor & 0x02)
		io_set_bit(LIGHT_FLOOR_IND1);
	else
		io_clear_bit(LIGHT_FLOOR_IND1);
	if (floor & 0x01)
		io_set_bit(LIGHT_FLOOR_IND2);
	else
		io_clear_bit(LIGHT_FLOOR_IND2);
}

int getButtonSignal(int floor, buttonType button){
	return io_read_bit(buttonMatrix[floor][button]);
}

void setButtonLamp(int floor, buttonType button, int status){
	if (status){
		io_set_bit(lampMatrix[floor][button]);
	}else{
		io_clear_bit(lampMatrix[floor][button]);
	}
}

int getButtonLamp(int floor,buttonType button){
	return io_read_bit(lampMatrix[floor][button]);
}
