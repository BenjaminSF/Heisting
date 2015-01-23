#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <pthread.c>

int recSock, sendSock;
int myPort = 20009;
int listenPort = 30000;

const int BUF_SIZE = 1024;

void* tFunc_sendSocket(){
	for (int i = 0; i++; i<5){	
		char *msg = "Hei paa deg, verden!";
		if (sendto(sendSock, msg, strlen(msg), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
			perror("Sending socket failed");
			return -1;
		}
		printf("Sending message...\n");
		sleep(1000);
	}
}

void* tFunc_recvSocket(){
	for (;;) {
		printf("Waiting for response...\n");
		int recvlen = recvfrom(recSock, buf, BUF_SIZE, 0, (struct sockaddr *)&remaddr, &remaddrLen);
		printf("Received bytes: %d\n", recvlen);
		if (recvlen > 0){
			buf[recvlen] = 0;
			printf("Received message: %s \n", buf);
		}
	}
}

int main(){
	unsigned char buf[BUF_SIZE];
	char *broadcastIP = "129.241.187.255";
	char *serverIP = "129.241.187.136";
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(listenPort);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(myPort);
	serveraddr.sin_addr.s_addr = inet_addr(broadcastIP);
	struct sockaddr_in remaddr;
	socklen_t remaddrLen = sizeof(remaddr);
	socklen_t recaddrLen = sizeof(myaddr);
	if ((recSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Socket not created\n");
		return -1;
	}
	if ((sendSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Socket not created\n");
		return -1;
	}
	
	int broadcastPermission = 1;
	if (setsockopt(sendSock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0){
		perror("sendSock broadcast enable failed");
		return -1;
	}

	if (bind(recSock,(struct sockaddr *)&myaddr,recaddrLen) < 0){
		perror("Failed to bind recSocket");
		return -1;
	}
	/*if (bind(sendSock,(struct sockaddr *)&myaddr, sizeof(myaddr)) < 0){
		perror("Failed to bind sendSocket");
		return -1;
	}*/
	
	pthread_t send_t, rec_t;
	pthread_create(&send_t, NULL, tFunc_sendSocket, NULL);
	pthread_create(&rec_t, NULL, tFunc_recSocket, NULL);
	//printf("Socket sent\n");
	sleep(5000);
	close(recSock);
	close(sendSock);
	return 0;
}
