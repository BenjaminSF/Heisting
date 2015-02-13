#include "elev.h"
#include <stdio.h>



int main() {

// Initialize hardware

	if (!elev_init()) {
		printf("Unable to initialize elevator hardware!\n");
		return 1;
}
	printf("Press STOP button to stop elevator and exit program.\n");
	//elev_set_motor_direction(DIRN_UP);
	int nextFloor = 0;
	int i;
	int queueActive = 0;
	while (1) {
	// Change direction when we reach top/bottom floor
		if (elev_get_floor_sensor_signal() == N_FLOORS - 1) {
			elev_set_motor_direction(DIRN_DOWN);
			//break;			
			//elev_set_motor_direction(DIRN_DOWN);
} 		/*else if (elev_get_floor_sensor_signal() == 0) {
			elev_set_motor_direction(DIRN_UP);
}*/
// Stop elevator and exit program if the stop button is pressed

		if (elev_get_stop_signal()) {
			elev_set_motor_direction(DIRN_STOP);
			break;
		}
		if (queueActive){
			if (elev_get_floor_sensor_signal() < nextFloor){
				elev_set_motor_direction(DIRN_UP);
			}else if (elev_get_floor_sensor_signal() > nextFloor){
				elev_set_motor_direction(DIRN_DOWN);
			}else if (elev_get_floor_sensor_signal() == nextFloor){
				elev_set_motor_direction(DIRN_STOP);
				queueActive = 0;
			}
		}
		for (i = 0; i < N_FLOORS; i++){
			if (elev_get_button_signal(BUTTON_COMMAND, i) == 1){
				nextFloor = i;
				queueActive = 1;
			}
	
	}
	
	}

return 0;
}
