#ifndef _publicTypes_
#define _publicTypes_

#define N_ORDERS 100
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

struct order{
	int dest;
	int buttonType;
	int elevator;
};



#endif