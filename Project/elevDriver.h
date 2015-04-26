#ifndef _elevDriver_
#define _elevDriver_

#include "publicTypes.h"

int elevDriver_initialize(void);
motorDirection getMotorDirection(void);
void setMotorDirection(motorDirection direction);
int getFloor(void);
void goToFloor(int floor);
int getButtonSignal(int floor, buttonType button);
void setDoorOpenLamp(int status);
void setFloorIndicator(int status);
void setButtonLamp(int floor, buttonType button, int status);

#endif
