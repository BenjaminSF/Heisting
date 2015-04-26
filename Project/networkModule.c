#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <linux/if_link.h>
#include "networkModule.h"
#include "fifoqueue.h"

static struct {
	char *localIP;
	char *broadcastIP;
	char *masterIP;
	int port;
	char **addrsList;
	int addrslistCounter;
	int masterStatus;
	sem_t masterSem;
	sem_t addrslistSem;
} info;


int initNetwork(){
	receiveQueue = new_fifoqueue();
	sendQueue = new_fifoqueue();
	sem_init(&(info.addrslistSem), 0, 1);
	sem_init(&(info.masterSem), 0, 1);

	//Finds the local machine's IP address
	struct ifaddrs *ifa,*ifaddr;
	int n;
	struct sockaddr_in *sa;
	char *tmpIP;
	if (getifaddrs(&ifaddr) < 0){
		perror("Failed to get local IP address.\n");
		return -1;
	}
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++){
		if ((ifa->ifa_addr->sa_family == AF_INET) && (!strcmp(ifa->ifa_name, "eth0"))){
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			tmpIP = inet_ntoa(sa->sin_addr);
		}
	}
	freeifaddrs(ifaddr);
	info.localIP = strdup(tmpIP);
	info.port = 20042;
	
	//Finds broadcast-IP:
	char *lastDot;
	lastDot = strrchr(tmpIP, '.');
	if (lastDot == NULL){
		printf("Feil IP-adresse format\n");
		return -1;
	}
	size_t index = (size_t)(lastDot - tmpIP);
	tmpIP[index+1] = '2';
	tmpIP[index+2] = '5';
	tmpIP[index+3] = '5';
	tmpIP[index+4] = '\0';
	info.broadcastIP = strdup(tmpIP);
	
	//Create message to be broadcasted
	BufferInfo sendInfo;
	encodeMessage(&sendInfo, 0, 0, MSG_CONNECT_SEND, 0, -1, -1);
	enqueue(sendQueue, &sendInfo, sizeof(BufferInfo));

	info.masterStatus = 0;
	info.addrsList = malloc(sizeof(char *) * N_ELEVATORS);
	info.addrsList[0] = strdup(info.localIP);
	info.addrslistCounter = 1;
	
	printf("Network initialization finished, local IP: %s\t Port: %d\t Broadcast IP: %s\n", info.localIP, info.port, info.broadcastIP);
	return inet_addr(info.localIP);
}


void* sendMessages(void *args){
	int sendSocket;
	struct sockaddr_in sendAddr;
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_port = htons(info.port);
	sendAddr.sin_addr.s_addr = inet_addr(info.broadcastIP);
	int socketPermission = 1;
	BufferInfo *msg = (BufferInfo *)malloc(sizeof(BufferInfo)+60);

	if ((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("SendSocket not created\n");
	}
	if (setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket broadcast enable failed\n");
	}
	if (setsockopt(sendSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket re-use port enable failed\n");
	}
	while(1){
		wait_for_content(sendQueue);
		dequeue(sendQueue, msg);
		if (sendto(sendSocket, (void *) msg,  sizeof(BufferInfo), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr)) < 0){
			perror("Sending socket failed\n");
		}
	}
	free(msg);
	return NULL;
}


void *receiveMessages(void *args){
	printf("Listen started\n");
	BufferInfo *tempMsg = (BufferInfo *)malloc(sizeof(BufferInfo));
	int port = info.port;
	struct timeval *timeout = NULL;
	int recSock;
	struct sockaddr_in remaddr;
	socklen_t remaddrLen = sizeof(remaddr);
	struct sockaddr_in myaddr = {.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY)};
	socklen_t myaddrLen = sizeof(myaddr);
	int socketPermission = 1;
	
	if ((recSock = socket(AF_INET,SOCK_DGRAM,0)) < 0){
		perror("Failed to create recSocket.\n");
	}
	if (setsockopt(recSock, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("recSocket reuse-port enable failed");
	}
	if (bind(recSock, (struct sockaddr *)&myaddr, myaddrLen) < 0){
		perror("Failed to bind recSocket.\n");
	}
	fd_set readfds;
	while (1){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock+1, &readfds, 0, 0, timeout);
		switch (event){
			case -1:
				printf("Switch/select for recSock failed.\n");
				break;
			case 0:
				printf("Timed out, should not happen\n");
				break;
			default:
				recvfrom(recSock, tempMsg, sizeof(BufferInfo), 0, (struct sockaddr *)&remaddr, &remaddrLen);
				enqueue(receiveQueue, tempMsg, sizeof(BufferInfo));
				break;
				
		}
	}
	close(recSock);
	return NULL;
}

void encodeMessage(BufferInfo *msg, int srcAddr, int dstAddr, int myState, int var1, int var2, int var3){
	if (srcAddr == 0){
		msg->srcAddr = inet_addr(info.localIP);
	}else{
		msg->srcAddr = srcAddr;
	}
	if (dstAddr == 0){
		msg->dstAddr = inet_addr(info.broadcastIP);

	}else{
		msg->dstAddr = dstAddr;
	}
	msg->myState = myState;
	switch(myState){
		case MSG_CONNECT_SEND:
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_CONNECT_RESPONSE:
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_ELEVSTATE:
			if (var1 != -1) msg->currentFloor = var1;
			if (var2 != -2) msg->nextFloor = var2;
			if (var3 != -1) msg->buttonType = var3;
			break;
		case MSG_BACKUP_ADD:
		case MSG_ADD_ORDER:
			if (var1 != -1) msg->nextFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			if (var3 != -1) msg->active = var3;
			if (var3 == 0) msg->active = getLocalIP();
			break;
		case MSG_SET_LAMP:
			if (var1 != -1) msg->currentFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			if (var3 != -1) msg->active = var3;
			break;
		case MSG_IM_ALIVE:
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_BACKUP_DELETE:
		case MSG_DELETE_ORDER:
			if (var1 != -1) msg->currentFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			if (var3 != -1) msg->active = var3;
			break;
		case MSG_DO_ORDER:
			if (var1 != -1) msg->nextFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			break;

	}
}

int addElevatorAddr(int newIP){
	int isInList = 0;
	struct in_addr tmp;
	tmp.s_addr = newIP;
	int i;
	sem_wait(&(info.addrslistSem));
	for (i = 0; i < info.addrslistCounter; i++){
		if (!strcmp(info.addrsList[i], inet_ntoa(tmp))){
			isInList = 1;
			break;
		}
	}
	if (!isInList){
		info.addrsList[info.addrslistCounter] = strdup(inet_ntoa(tmp));
		info.addrslistCounter++;
	}
	sem_post(&(info.addrslistSem));
	return isInList;
}

int addrsList(int pos){
	sem_wait(&(info.addrslistSem));
	int tmp = inet_addr(info.addrsList[pos]);
	sem_post(&(info.addrslistSem));
	return tmp;
}

void resetAddrsList(){
	sem_wait(&(info.addrslistSem));
	info.addrslistCounter = 1;
	sem_post(&(info.addrslistSem));
}

void resetAddr(int IP){
	int pos;
	sem_wait(&(info.addrslistSem));
	for(pos=0;pos<info.addrslistCounter;pos++){
		if (inet_addr(info.addrsList[pos]) == IP){
			char* tmpChar = strdup(info.addrsList[info.addrslistCounter-1]);
			info.addrsList[info.addrslistCounter-1] = strdup(info.addrsList[pos]);
			info.addrsList[pos] = strdup(tmpChar);
			info.addrslistCounter--;
			break;
		}
	}
	sem_post(&(info.addrslistSem));
}

int getAddrsCount(){
	sem_wait(&(info.addrslistSem));
	int tmp = info.addrslistCounter;
	sem_post(&(info.addrslistSem));
	return tmp;
}

int getLocalIP(){
	sem_wait(&(info.addrslistSem));
	int tmp = inet_addr(info.localIP);
	sem_post(&(info.addrslistSem));
	return tmp;
}

int getBroadcastIP(){
	sem_wait(&(info.addrslistSem));
	int tmp = inet_addr(info.broadcastIP);
	sem_post(&(info.addrslistSem));
	return tmp;
}

int getMasterStatus(){
	sem_wait(&(info.masterSem));
	int tmp = info.masterStatus;
	sem_post(&(info.masterSem));
	return tmp;
}

void setMasterIP(int newMasterIP){
	printf("Set master IP: %d\n", newMasterIP);
	struct in_addr tmp;
	tmp.s_addr = newMasterIP;
	sem_wait(&(info.masterSem));
	info.masterIP = strdup(inet_ntoa(tmp));
	if (newMasterIP == getLocalIP()){
		info.masterStatus = 1;
		printf("This elevator is master.\n");
	}else{
		info.masterStatus = 0;
		printf("This elevator is slave.\n");
	}
	sem_post(&(info.masterSem));
}
