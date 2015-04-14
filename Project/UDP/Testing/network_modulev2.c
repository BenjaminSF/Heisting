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
#define BUF_SIZE 1024

//char bufMessage[BUF_SIZE];
//char bufSend[BUF_SIZE];
fifoqueue_t* receiveQueue;
fifoqueue_t* sendBuffer;

//All values set by init_network
static struct {
	char *localIP;
	char *broadcastIP;
	int port;
	char **addrsList; //Ikke implementert enda
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


enum bufferState{
	MSG_CONNECT_SEND,
	MSG_CONNECT_RESPONSE,
	MSG_ELEVSTATE,
	MSG_ELEV_UP,
	MSG_ELEV_DOWN,
	MSG_ELEV_COMMAND
};

struct bufferInfo{
	char *srcAddr;
	char *dstAddr;
	int masterStatus;
	enum bufferState myState;
};

void* send_message(void *args){
	printf("Sending started\n");
	struct ListenParams myArgs = *((struct ListenParams*)(args));
	char msg[BUF_SIZE];
	dequeue(sendQueue, msg);
	printf("Sending message: %s\n", msg);

	int sendSocket;
	struct sockaddr_in sendAddr;
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_port = htons(info.port);
	sendAddr.sin_addr.s_addr = inet_addr(info.broadcastIP);
	int socketPermission = 1;

	if ((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("SendSocket not created\n");
	}
	if (setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket broadcast enable failed\n");
	}
	if (setsockopt(sendSocket, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket re-use port enable failed\n");
	}
	int i;
	for (i = 0; i < 10; i++){
		if (sendto(sendSocket, msg, strlen(msg), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr)) < 0){
			perror("Sending socket failed\n");
		}
		if (sendto(sendSocket, msg, strlen(msg), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr)) < 0){
			perror("Sending socket failed\n");
		}
		usleep(100000);
	}
	printf("Sending finished\n");
	return;
}


void *listen_for_messages(void *args){
	printf("Listen started\n");
	char tempString[BUF_SIZE];
	struct ListenParams *myArgs = ((struct ListenParams*)(args));
	int recSock;
	struct sockaddr_in remaddr;
	socklen_t remaddrLen = sizeof(remaddr);
	struct sockaddr_in myaddr = {.sin_family = AF_INET, .sin_port = htons(myArgs->port), .sin_addr.s_addr = htonl(INADDR_ANY)};
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

	printf("Test mottak, port: %d\n", (myArgs)->port);
	struct timeval timeout = {.tv_sec = 0, .tv_usec = (myArgs)->timeoutMs * 1000};
	fd_set readfds;
	//prepare socket
	int notDone = 1;
	while (notDone){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock+1, &readfds, 0, 0, &timeout);
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
				printf("Recieving\n");
				memset(tempString, '\0', BUF_SIZE);
				recvfrom(recSock, tempString, BUF_SIZE, 0, (struct sockaddr *)&remaddr, &remaddrLen);
				enqueue(receiveQueue, tempString, BUF_SIZE);
				sem_post(&myArgs->readReady);
				
		}
	}
	myArgs->finished = 1;
	sem_post(&(myArgs->readReady));
	printf("Listen finished\n");
	printf("Test mottak, port: %d\n", (myArgs)->port);
	return;
}

int init_network(){
	receiveQueue = new_fifoqueue();
	sendBuffer = new_fifoqueue();
	
	//Finds the local machine's IP address
	printf("Start init\n");
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
	info.port = 20011; //Set to a static value for port, could implement and call a function if necessary
	
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
	
	//Create message to be broadcasted
	char connect2meMsg[BUF_SIZE];
	struct bufferInfo sendInfo = {.srcAddr = info.localIP, .dstAddr = info.broadcastIP};
	sendInfo.masterStatus = 0;
	sendInfo.myState = MSG_CONNECT_SEND;
	encodeMessage(connect2meMsg, sendInfo);
	enqueue(sendBuffer, connect2meMsg, BUF_SIZE)
	printf("connect2meMsg: %s\n", connect2meMsg);
	
	//Start listening for responses
	struct ListenParams params = {.port = info.port, .timeoutMs = 5000, .finished = 0};
	sem_init(&(params.readReady),0,0);
	struct ListenParams sendParams = {.port = info.port};

	printf("Starting pthreads\n");
	pthread_t findOtherElevs, findElevsSend;
	pthread_create(&findOtherElevs, NULL, &listen_for_messages, &params); //Listen
	pthread_create(&findElevsSend, NULL, send_message, &sendParams);	//Send
	int addrslistCounter = 0;
	struct bufferInfo bufInfo;

	char tmpResponseMsg[BUF_SIZE];
	printf("Mutex locked, waiting for cond\n");
	while(1){}//pthread_kill(findOtherElevs, 0) != ESRCH){	//Listening

		sem_wait(&(params.readReady)); //To avoid blocking on wait_for_content()
		if (params.finished == 1){
			break;
		}
		wait_for_content(receiveQueue);
		
		memset(tmpResponseMsg, '\0', BUF_SIZE);
		dequeue(receiveQueue, tmpResponseMsg);
		bufInfo = decodeMessage(tmpResponseMsg);
		printf("Received: %s\n", tmpResponseMsg);
		if (bufInfo.myState == MSG_CONNECT_RESPONSE){ //Only use related messages
			//info.addrsList[addrslistCounter] = bufInfo.srcAddr;
			addrslistCounter++;
			if (bufInfo.masterStatus == 1){
				//master not available
				//current master is bufInfo.srcAddr
			}
			//add buffer to address list
		}
	}

	printf("Finished listening\n");
	//Finished listening
	pthread_join(findOtherElevs, NULL);
	pthread_join(findElevsSend, NULL);
	sem_destroy(&(params.readReady));
	
	return 0;
}

struct bufferInfo decodeMessage(char *buffer){
	struct bufferInfo msg;
	//For testing only:
	msg.srcAddr = "192.128.187.111";
	msg.dstAddr = "192.128.187.123";
	msg.masterStatus = 1;
	msg.myState = MSG_CONNECT_RESPONSE;
	//End: Testing

	return msg;
}

void encodeMessage(char *buffer, struct bufferInfo information){
	char* melding = "testing 12";
	strcpy(buffer,melding);
}