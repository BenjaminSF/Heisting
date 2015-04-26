#ifndef _networkModule_
#define _networkModule_

#include <stdio.h>
#include "fifoqueue.h"
#include "publicTypes.h"

fifoqueue_t* receiveQueue;
fifoqueue_t* sendQueue;

int initNetwork();
void* sendMessages(void *args);
void* receiveMessages(void *args);
void encodeMessage(BufferInfo *msg, int srcAddr,int dstAddr, int myState, int var1, int var2, int var3);

int addElevatorAddr(int newIP);
int addrsList(int pos);
void resetAddrsList();
void resetAddr(int IP);
int getLocalIP();
int getBroadcastIP();
int getAddrsCount();
int getMaster();
void setMasterIP(int newMasterIP);

#endif