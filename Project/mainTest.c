#include "elevDriver.h"
#include <stdio.h>


int main() {
    // Initialize hardware
    if (!elevDriver_initialize()) {
        printf("Unable to initialize elevator hardware!\n");
        return 1;
    }

    printf("Press STOP button to stop elevator and exit program.\n");

    setMotorDirection(DIRN_UP);

    while (1) {
        // Change direction when we reach top/bottom floor
        if (getFloor() == N_FLOORS - 1) {
            setMotorDirection(DIRN_DOWN);
        } else if (getFloor() == 0) {
            setMotorDirection(DIRN_UP);
        }

        // Stop elevator and exit program if the stop button is pressed
        if (isStopped()) {
            setMotorDirection(DIRN_STOP);
            break;
        }
    }

    return 0;
}

