#ifndef _publicTypes_
#define _publicTypes_

#define N_ORDERS 100
#define N_FLOORS 4
#define N_ELEVATORS 5
#define N_BUTTONS 3

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

typedef struct Order{
	int dest;
	int buttonType;
	int elevator;
}Order;

enum bufferState{
	MSG_CONNECT_SEND,
	MSG_CONNECT_RESPONSE,
	MSG_ELEVSTATE,
	MSG_ADD_ORDER,
	MSG_DO_ORDER,
	MSG_SET_LAMP,
	MSG_IM_ALIVE,
	MSG_DELETE_ORDER,
	MSG_MASTER_REQUEST,
	MSG_MASTER_PROPOSAL,
	MSG_ADDR_REQUEST,
	MSG_ADDR_RESPONSE,
	MSG_BACKUP_ADD,
	MSG_BACKUP_DELETE
};

typedef struct BufferInfo{
	int srcAddr;
	int dstAddr;
	int masterStatus;
	enum bufferState myState;
	int active;
	int currentFloor;
	int nextFloor;
	int buttonType;
} BufferInfo;


#endif