#ifndef _networkModule_
#define _networkModule_

#include <stdio.h>
#include "fifoqueue.h"
#include "publicTypes.h"

fifoqueue_t* receiveQueue;
fifoqueue_t* sendQueue;

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
void resetAddr(int IP);

#endif