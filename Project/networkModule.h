#ifndef _networkModule_
#define _networkModule_

#include <stdio.h>
#include "fifoqueue.h"

fifoqueue_t* receiveQueue;
fifoqueue_t* sendQueue;

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
	MSG_CONFIRM_ORDER,
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

int init_network();
void* send_message(void *args);
void* listen_for_messages(void *args);

//encodeMessage: Set the relevant fields in a BufferInfo struct all at once
//Set var# = -1 when not applicable
void encodeMessage(BufferInfo *msg, int srcAddr,int dstAddr, int myState, int var1, int var2, int var3);
int addElevatorAddr(int newIP);
int addrsList(int pos);
int getLocalIP();
int getBroadcastIP();
int getAddrsCount();
int getMaster();
void resetAddrsList();
void setMasterIP(int newMasterIP);
void setMaster(int newMaster);

#endif