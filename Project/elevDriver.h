#ifndef __INCLUDE_DRIVER_H__
#define __INCLUDE_DRIVER_H__

// Number of floors

#define N_FLOORS 4

typedef enum tag_motorDirection {
DIRN_DOWN = -1,
DIRN_STOP = 0,
DIRN_UP = 1
} motorDirection;
typedef enum tag_lampType {
	BUTTON_CALL_UP = 0,
	BUTTON_CALL_DOWN = 1,
	BUTTON_COMMAND = 2
} buttonType;

int elevDriver_initialize(void);

motorDirection getMotorDirection(void);

void setMotorDirection(motorDirection direction);

void goToFloor(int floor);

void setDoorOpenLamp(int status);

int isElevObstructed(void);

int isElevStopped(void);

void setStopLamp(int status);

int getFloor(void);

void setFloorIndicator(int status);

int getButtonSignal(int floor, buttonType button);

void setButtonLamp(int floor, buttonType button, int status);

#endif // #ifndef __INCLUDE_DRIVER_H__
