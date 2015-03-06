#include "channels.h"
#include "io.h"
#include "elevDriver.h"
#include <assert.h>
#include <stdlib.h>

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
			elev_set_button_lamp(BUTTON_CALL_DOWN, i, 0);
		if (i != N_FLOORS - 1)
			elev_set_button_lamp(BUTTON_CALL_UP, i, 0);
			elev_set_button_lamp(BUTTON_COMMAND, i, 0);
	}
	// Clear stop lamp, door open lamp, and set floor indicator to ground floor.
	elev_set_stop_lamp(0);
	elev_set_door_open_lamp(0);
	elev_set_floor_indicator(0);
	// Return success.
	return 1;
}
void setMotorDirection(elev_motor_direction_t dirn) {
	if (dirn == 0){
		io_write_analog(MOTOR, 0);
}	else if (dirn > 0) {
		io_clear_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
} 	else if (dirn < 0) {
		io_set_bit(MOTORDIR);
		io_write_analog(MOTOR, 2800);
}
}

void setFloor();

void setDoorOpenLamp(int status){
	if status{
		io_set_bit(LIGHT_DOOR_OPEN);
	}else{
		io_clear_bit(LIGHT_DOOR_OPEN);
	}
}

int isElevObstructed(){
	return io_read_bit(OBSTRUCTION);
}