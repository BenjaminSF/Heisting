#ifndef _elevDriver_
#define _elevDriver_

#include "publicTypes.h"

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

#endif
