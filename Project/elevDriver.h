#ifndef __INCLUDE_DRIVER_H__
#define __INCLUDE_DRIVER_H__

#include "publicTypes.h"
// Number of floors

#define N_FLOORS 4

int elevDriver_initialize(void);

motorDirection getMotorDirection(void);

void setMotorDirection(motorDirection direction);

void goToFloor(int floor);

void setDoorOpenLamp(int status);

int isObstructed(void);

int isStopped(void);

void setStopLamp(int status);

int getFloor(void);

void setFloorIndicator(int status);

int getButtonSignal(int floor, buttonType button);

void setButtonLamp(int floor, buttonType button, int status);

#endif // #ifndef __INCLUDE_DRIVER_H__
