#ifndef _network_modulev2_
#define _network_modulev2_

#include <stdio.h>
#include <pthread.h>
#include "fifoqueue.h"

fifoqueue_t* receiveQueue;
fifoqueue_t* sendQueue;

enum bufferState{
	MSG_CONNECT_SEND,
	MSG_CONNECT_RESPONSE,
	MSG_ELEVSTATE,
	MSG_ELEV_UP,
	MSG_ELEV_DOWN,
	MSG_ELEV_COMMAND,
	MSG_ADD_ORDER,
	MSG_GET_ORDER,
	MSG_DO_ORDER,
	MSG_SET_LAMP,
	MSG_IM_ALIVE,
	MSG_DELETE_ORDER,
	MSG_MASTER_REQUEST,
	MSG_MASTER_PROPOSAL,
	MSG_CONFIRM_ORDER,
	MSG_ADDR_REQUEST,
	MSG_ADDR_RESPONSE,
	MSG_BACKUP_ADD,
	MSG_BACKUP_DELETE
};

//extern pthread_mutex_t masterMutex;
//extern pthread_mutexattr_t mastermattr;

typedef struct BufferInfo{
	int srcAddr;
	int dstAddr;
	int masterStatus;
	enum bufferState myState;
	int active;
	int currentFloor;
	int nextFloor;
	//int elevator; //?
	int buttonType;
} BufferInfo;

int init_network();
void* send_message(void *args);
void* listen_for_messages(void *args);
BufferInfo decodeMessage(char *buffer);
//int addNewOrder(struct order newOrder, int currentFloor, int nextFloor);
//int getNewOrder(int currentFloor, int nextFloor);

//encodeMessage: Set the relevant fields in a BufferInfo struct all at once
//Set var# = -1 when not applicable
void encodeMessage(BufferInfo *msg, int srcAddr,int dstAddr, int myState, int var1, int var2, int var3);
int getLocalIP();
int getBroadcastIP();
void setMasterIP(int IP);
int addElevatorAddr(int newIP);
int getAddrsCount();
int addrsList(int pos);
int getMaster();
void setMaster(int x);
void resetAddrsList();


#endif