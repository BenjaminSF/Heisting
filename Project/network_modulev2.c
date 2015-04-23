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
#include "network_modulev2.h"
#include "fifoqueue.h"
#include "costFunction.h"
#define BUF_SIZE 1024
#define MAX_ELEVS 5

//char bufMessage[BUF_SIZE];
//char bufSend[BUF_SIZE];


//All values set by init_network
static struct {
	char *localIP;
	char *broadcastIP;
	char *masterIP;
	int port;
	char **addrsList; //Ikke implementert enda
	int addrslistCounter;
	int masterStatus; //Ikke implementert enda
} info;

struct ListenParams{
	int port;
	int timeoutMs;
	int finished;
	//struct lock rwLock;
	//struct condition available;
	//pthread_mutex_t rwLock;
	sem_t readReady;
};


void* send_message(void *args){
	printf("Sending started\n");
	//struct ListenParams myArgs = *((struct ListenParams*)(args));
	//char msg[BUF_SIZE];

	int sendOnce = (intptr_t) args;
	int sendSocket;
	struct sockaddr_in sendAddr;
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_port = htons(info.port);
	sendAddr.sin_addr.s_addr = inet_addr(info.broadcastIP);
	int socketPermission = 1;
	printf("BufferInfo: %lu, int: %lu, enum: %lu\n", sizeof(BufferInfo), sizeof(int), sizeof(char));
	BufferInfo *msg = (BufferInfo *)malloc(sizeof(BufferInfo)+60);
	while(1){
		wait_for_content(sendQueue);
		dequeue(sendQueue, msg);
		//printf("Sending message: %d\n", msg->myState);
		//printf("Send size srcAddr: %lu, %d\n", sizeof(msg), msg->srcAddr);

		if ((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
			perror("SendSocket not created\n");
		}
		if (setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (void*)&socketPermission, sizeof(socketPermission)) < 0){
			perror("sendSocket broadcast enable failed\n");
		}
		if (setsockopt(sendSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
			perror("sendSocket re-use port enable failed\n");
		}
		if (sendto(sendSocket, (void *) msg,  sizeof(BufferInfo), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr)) < 0){
			perror("Sending socket failed\n");
		}
		if (sendOnce == 1){
			break;
		}
	}
	free(msg);
	printf("Sending finished\n");
	return;
}


void *listen_for_messages(void *args){
	printf("Listen started\n");
	BufferInfo *tempMsg = (BufferInfo *)malloc(sizeof(BufferInfo));
	int nullParam = 0;
	int port, timeoutMs;
	struct ListenParams *myArgs;

	struct timeval *timeout = malloc(sizeof(struct ListenParams));
	if (args == NULL){
		nullParam = 1;
		port = info.port;
		timeoutMs = 0;
		/*struct ListenParams *myArgs;
		printf("sem_init\n");
		sem_init(&(myArgs->readReady), 0, 0);
		printf("sem_init done\n");
		myArgs->port = info.port;
		myArgs->timeoutMs = 0;
		myArgs->finished = 0;*/
		
		timeout = NULL;
		
	}else{
		myArgs = ((struct ListenParams*)(args));
		port = myArgs->port;
		timeoutMs = myArgs->timeoutMs;
		timeout->tv_sec = 0;
		timeout->tv_usec = timeoutMs * 1000;

	}
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

	printf("Test mottak, port: %d\n", port);

	fd_set readfds;
	//prepare socket
	int notDone = 1;
	while (notDone){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock+1, &readfds, 0, 0, timeout);
		switch (event){
			case -1:
				printf("Switch/select for recSock failed.\n");
				notDone = 0; 
				break;
			case 0:
				close(recSock);
				notDone = 0;
				printf("Timed out\n");
				break;
			default:
				//printf("Recieving\n");
				//memset(tempString, '\0', BUF_SIZE);
				recvfrom(recSock, tempMsg, sizeof(BufferInfo), 0, (struct sockaddr *)&remaddr, &remaddrLen);
				printf("ListenReceived: %d\n", tempMsg->myState);
				//printf("size, struct: %d, srcAddr: %lu, myState: %lu\n", sizeof(tempMsg), sizeof(int*), sizeof(char));
				enqueue(receiveQueue, tempMsg, BUFFER_SIZE);
				if (!nullParam) sem_post(&myArgs->readReady);
				
		}
	}
	if (!nullParam){
		myArgs->finished = 1;
		sem_post(&(myArgs->readReady));
	}
	printf("Listen finished\n");
	printf("Test mottak, port: %d\n", port);
	return;
}

int init_network(){
	receiveQueue = new_fifoqueue();
	sendQueue = new_fifoqueue();
	
	//Finds the local machine's IP address
	printf("Start init_network()\n");
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *tmpIP;
	//tmpIP = malloc(20 * sizeof(char));
	if (getifaddrs(&ifap) < 0){
		perror("Failed to get local IP address.\n");
		return -1;
	}
	for (ifa = ifap; ifa; ifa = ifa->ifa_next){
		if ((ifa->ifa_addr->sa_family == AF_INET) && (!strcmp(ifa->ifa_name, "eth0"))){
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			tmpIP = inet_ntoa(sa->sin_addr);
			//Unhandled exception: "eth0" does not exist"
		}
	}
	freeifaddrs(ifap);
	
	//set IP info for use by other functions
	info.localIP = strdup(tmpIP);
	info.port = 20023; //Set to a static value for port, could implement and call a function if necessary
	
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
	printf("Lokalt: IP: %s\t Port: %d\t Broadcast: %s\n", info.localIP, info.port, info.broadcastIP);
	printf("Lokal IP int: %d\n", inet_addr(info.localIP));
	
	//Create message to be broadcasted
	//char connect2meMsg[BUF_SIZE];
	BufferInfo sendInfo;
	sendInfo.srcAddr = inet_addr(info.localIP);
	sendInfo.dstAddr = inet_addr(info.broadcastIP);
	sendInfo.masterStatus = 0;
	sendInfo.myState = MSG_CONNECT_SEND;
	//encodeMessage(connect2meMsg, sendInfo);
	enqueue(sendQueue, &sendInfo, BUFFER_SIZE);
	//printf("connect2meMsg: %s\n", connect2meMsg);
	
	//Start listening for responses
	struct ListenParams params = {.port = info.port, .timeoutMs = 5000, .finished = 0};
	sem_init(&(params.readReady),0,0);
	//struct ListenParams sendParams = {.port = info.port};
	int sendOnce = 1;

	printf("Starting pthreads\n");
	pthread_t findOtherElevs, findElevsSend;
	pthread_create(&findOtherElevs, NULL, &listen_for_messages, &params); //Listen
	pthread_create(&findElevsSend, NULL, send_message, (void *) (intptr_t) sendOnce);	//Send
	info.addrslistCounter = 0;
	BufferInfo bufInfo;

	//char tmpResponseMsg[BUF_SIZE];
	//printf("Mutex locked, waiting for cond\n");
	info.masterStatus = 1;
	info.addrsList = malloc(sizeof(char *) * MAX_ELEVS);
	int isInList;
	while(1){//pthread_kill(findOtherElevs, 0) != ESRCH){	//Listening

		sem_wait(&(params.readReady)); //To avoid blocking on wait_for_content()
		if (params.finished == 1){
			break;
		}
		wait_for_content(receiveQueue);
		
		//memset(tmpResponseMsg, '\0', BUF_SIZE);
		dequeue(receiveQueue, &bufInfo);
		//bufInfo = decodeMessage(tmpResponseMsg);
		printf("Received: %d\n", bufInfo.srcAddr);
		printf("Received: %d\n", bufInfo.myState);
		if (bufInfo.myState == MSG_CONNECT_RESPONSE){ //Only use related messages
			printf("Adding to list\n");
			info.masterStatus = 0;
			struct in_addr tmp;
			tmp.s_addr = bufInfo.srcAddr;
			if (bufInfo.masterStatus == 1){
				info.masterIP = strdup(inet_ntoa(tmp));
				//master not available
				//current master is bufInfo.srcAddr
			}
			isInList = 0;
			int i;
			for (i = 0; i < MAX_ELEVS; i++){
				if (!strcmp(info.addrsList[i], inet_ntoa(tmp))){
					isInList = 1;
					break;
				}
			}
			if (!isInList){
				info.addrsList[info.addrslistCounter] = strdup(inet_ntoa(tmp));
				info.addrslistCounter++;
			}
			

			//add buffer to address list
		}
	}

	//printf("Finished listening\n");
	//Finished listening
	pthread_join(findOtherElevs, NULL);
	pthread_join(findElevsSend, NULL);
	sem_destroy(&(params.readReady));
	
	if (info.masterStatus == 1){
		info.masterIP = strdup(info.localIP);
	}
	printf("Init network finished\n");
	return info.masterStatus;
}

BufferInfo decodeMessage(char *buffer){
	BufferInfo msg;
	//For testing only:
	msg.srcAddr = inet_addr("192.128.187.111");
	msg.dstAddr = inet_addr("192.128.187.123");
	msg.masterStatus = 1;
	msg.myState = MSG_CONNECT_RESPONSE;
	//End: Testing

	return msg;
}

void encodeMessage(BufferInfo *msg, int srcAddr, int dstAddr, int myState, int var1, int var2, int var3){
	printf("Encoding message------------------------------------------------------\n");
	if (srcAddr == 0){
		printf("strcpy: %d , %s\n",msg->srcAddr,info.localIP);

		msg->srcAddr = inet_addr(info.localIP);
	}else{
		msg->srcAddr = srcAddr;
	}
	if (dstAddr == 0){
		msg->dstAddr = inet_addr(info.broadcastIP);

	}else{
		msg->dstAddr = dstAddr;
	}
	printf("Encode: IPs done\n");
	msg->myState = myState;
	switch(myState){
		case MSG_CONNECT_SEND:
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_CONNECT_RESPONSE:
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_ELEVSTATE:
			if (var1 != -1) msg->active = var1;
			if (var2 != -1) msg->currentFloor = var2;
			if (var3 != -1) msg->nextFloor = var3;
			if ((var3 != -1) && (var2 != -1)){
				int dir = var3 - var2;
				if (dir > 0) msg->direction = 1;
			}
			break;
		case MSG_ADD_ORDER:
			if (var1 != -1) msg->nextFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			break;
		case MSG_GET_ORDER:
			if (var1 != -1) msg->active = var1;
			if (var2 != -1) msg->currentFloor = var2;
			if (var3 != -1) msg->nextFloor = var3;
			if ((var3 != -1) && (var2 != -1)){
				int dir = var3 - var2;
				if (dir > 0) msg->direction = 1;
			}
			break;
		case MSG_SET_LAMP:
			if (var1 != -1) msg->currentFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			if (var3 != -1) msg->active = var3;
			break;
		case MSG_IM_ALIVE:
			printf("Encode: I'm alive\n");
			if (var1 != -1) msg->masterStatus = var1;
			break;
		case MSG_DELETE_ORDER:
			if (var1 != -1) msg->currentFloor = var1;
			if (var2 != -1) msg->buttonType = var2;
			if (var3 != -1) msg->active = var3;
			break;


	}

}

int getLocalIP(){
	printf("getLocalIP\n");
	return inet_addr(info.localIP);
}

int getBroadcastIP(){
	printf("getBroadcastIP\n");
	return inet_addr(info.broadcastIP);
}

void setMasterIP(int x){
	printf("setMasterIP\n");
	struct in_addr tmp;
	tmp.s_addr = x;
	info.masterIP = strdup(inet_ntoa(tmp));
}

void addElevatorAddr(int newIP){
	printf("addElevatorAddr\n");
	int isInList = 0;
	struct in_addr tmp;
	tmp.s_addr = newIP;
	int i;
	for (i = 0; i < MAX_ELEVS; i++){
		if (!strcmp(info.addrsList[i], inet_ntoa(tmp))){
			isInList = 1;
			break;
		}
	}
	if (!isInList){
		info.addrsList[info.addrslistCounter] = strdup(inet_ntoa(tmp));
		info.addrslistCounter++;
	}
}

int getAddrsCount(){
	printf("getAddrsCount\n");
	return info.addrslistCounter;
}

int addrsList(int i){
	printf("addrsList\n");
	return inet_addr(info.addrsList[i]);
}