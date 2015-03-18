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
#include "network_modulev2.h"
#define BUF_SIZE 1024

char bufMessage[BUF_SIZE];
char bufSend[BUF_SIZE];

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
	//struct lock rwLock;
	//struct condition available;
	pthread_mutex_t rwLock;
	pthread_cond_t readReady;
};


enum bufferState{
	MSG_CONNECT_SEND,
	MSG_CONNECT_RESPOND,
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
	pthread_mutex_lock(&myArgs.rwLock);
	strcpy(msg, bufSend);
	pthread_cond_signal(&myArgs.readReady);
	pthread_mutex_unlock(&myArgs.rwLock);

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
	if (sendto(sendSocket, msg, strlen(msg), 0, (struct sockaddr*)&sendAddr, sizeof(sendAddr)) < 0){
		perror("Sending socket failed\n");
	}
	printf("Sending finished\n");
	return;
}


void *listen_for_messages(void *args){
	printf("Listen started\n");
	struct ListenParams myArgs = *((struct ListenParams*)(args));
	int recSock;
	struct sockaddr_in remaddr;
	socklen_t remaddrLen = sizeof(remaddr);
	struct sockaddr_in myaddr = {.sin_family = AF_INET, .sin_port = htons(myArgs.port), .sin_addr.s_addr = htonl(INADDR_ANY)};
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

	printf("Test mottak, port: %d\n", (myArgs).port);
	struct timeval timeout = {.tv_sec = 0, .tv_usec = (myArgs).timeoutMs * 1000};
	fd_set readfds;
	//prepare socket
	int notDone = 1;
	while (notDone){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock, &readfds, 0, 0, &timeout);
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
				pthread_mutex_lock((&myArgs.rwLock));
				
				recvfrom(recSock, bufMessage, BUF_SIZE, 0, (struct sockaddr *)&remaddr, &remaddrLen);
				pthread_cond_signal(&myArgs.readReady);
				pthread_mutex_unlock((&myArgs.rwLock));
		}
	}
	printf("Listen finished\n");
	return;
}

int init_network(){
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
	info.port = 20005; //Set to a static value for port, could implement and call a function if necessary
	
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
	struct bufferInfo sendInfo = {.srcAddr = info.localIP, .dstAddr = info.broadcastIP};
	sendInfo.masterStatus = 0;
	sendInfo.myState = MSG_CONNECT_SEND;
	encodeMessage(bufSend, sendInfo);

	//Start listening for responses
	struct ListenParams params = {.port = info.port, .timeoutMs = 2000};
	pthread_mutex_init(&(params.rwLock), NULL);
	pthread_cond_init(&(params.readReady), NULL);

	struct ListenParams sendParams = {.port = info.port};
	pthread_mutex_init(&(sendParams.rwLock), NULL);
	pthread_cond_init(&(sendParams.readReady), NULL);
	

	printf("Starting pthreads\n");
	pthread_t findOtherElevs, findElevsSend;
	pthread_create(&findOtherElevs, NULL, &listen_for_messages, &params); //Listen
	pthread_create(&findElevsSend, NULL, send_message, &sendParams);	//Send
	int addrslistCounter = 0;
	struct bufferInfo bufInfo;
	while(pthread_kill(findOtherElevs, 0) != ESRCH){	//Listening
		printf("Entered while-loop\n");
		pthread_mutex_lock(&(params.rwLock));
		printf("Mutex locked, waiting for cond\n");
		pthread_cond_wait(&(params.readReady), &(params.rwLock));
		bufInfo = decodeMessage(bufMessage);
		printf("Decoded\n");
		pthread_mutex_unlock(&(params.rwLock));
		if (bufInfo.myState == MSG_CONNECT_RESPOND){ //Only use related messages
		
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
	pthread_mutex_destroy(&(params.rwLock));
	pthread_cond_destroy(&(params.readReady));
	pthread_mutex_destroy(&(sendParams.rwLock));
	pthread_cond_destroy(&(sendParams.readReady));
	return 0;
}

struct bufferInfo decodeMessage(char *buffer){
	struct bufferInfo msg;
	//For testing only:
	msg.srcAddr = "192.128.187.111";
	msg.dstAddr = "192.128.187.123";
	msg.masterStatus = 1;
	msg.myState = MSG_CONNECT_RESPOND;
	//End: Testing

	return msg;
}

void encodeMessage(char *buffer, struct bufferInfo information){
	buffer = "Testing:12:12";

}