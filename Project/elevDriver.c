#include "channels.h"
#include "io.h"
#include "elevDriver.h"
#include <assert.h>
#include <stdlib.h>

#define N_BUTTONS 3

static const int lamp_channel_matrix[N_FLOORS][N_BUTTONS] = {
	{LIGHT_UP1, LIGHT_DOWN1, LIGHT_COMMAND1},
	{LIGHT_UP2, LIGHT_DOWN2, LIGHT_COMMAND2},
	{LIGHT_UP3, LIGHT_DOWN3, LIGHT_COMMAND3},
	{LIGHT_UP4, LIGHT_DOWN4, LIGHT_COMMAND4},
};
static const int button_channel_matrix[N_FLOORS][N_BUTTONS] = {
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
			setButtonLamp(BUTTON_CALL_DOWN, i, 0);
		if (i != N_FLOORS - 1)
			setButtonLamp(BUTTON_CALL_UP, i, 0);
			setButtonLamp(BUTTON_COMMAND, i, 0);
	}
	// Clear stop lamp, door open lamp, and set floor indicator to ground floor.
	setStopLamp(0);
	setDoorOpenLamp(0);
	setFloorIndicator(0);
	// Return success.
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

void setFloor(void);

void setDoorOpenLamp(int status){
	if status{
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
	if status{
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
int isbuttonSignalValid(int floor, butttonType button){
	assert(floor>= 0 && floor < N_FLOORS);
	assert(!(button == BUTTON_CALL_UP && floor == N-1) && !(button == BUTTON_CALL_DOWN && floor == 0))
	assert(button == BUTTON_CALL_DOWN || button == BUTTON_CALL_UP || BUTTON_COMMAND);
	return 1;
}
int getButtonLamp(int floor, buttonType button){
	if isbuttonSignalValid(floor,button){
		return io_read_bit(button_channel_matrix[floor][button]);
}
	return -1;
}
void setButtonLamp(int floor, buttonType button, int status){
	if isbuttonSignalValid(floor,button){
			if (status){
				io_set_bit(lamp_channel_matrix[floor][button]);
			}else{
				io_clear_bit(lamp_channel_matrix[floor][button]);
			}
	}
}

