#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#define BUF_SIZE 1024
unsigned char bufMessage[BUF_SIZE];

void sendBroadcast(int statusMsg){
	char *broadcastIP = "129.241.187.255";
	int myPort = 20011;
	int sendSock;
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(myPort);
	serveraddr.sin_addr.s_addr = inet_addr(broadcastIP);
	int socketPermission = 1;
	if ((sendSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("SendSocket not created\n");
	}
	if (setsockopt(sendSock, SOL_SOCKET, SO_BROADCAST, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket broadcast enable failed");
	}
	if (setsockopt(sendSock, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket reuse-port enable failed");
	}
	char msg[20];
	if (statusMsg == 0){	
		sprintf(msg, "I'm alive!");
	}else{
		sprintf(msg, "%d", statusMsg);
	}
	if (sendto(sendSock, msg, strlen(msg), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("Sending socket failed");
	}
		
		


	close(sendSock);
	return;

}


void primaryFunc(int counter){
	printf("Starting primary.\n");
	time_t startTime, endTime;
	time(&startTime);	
	while(1){
		time(&endTime);
		if (difftime(endTime, startTime) > 1){
			sendBroadcast(0);
			time(&startTime);
			printf("Primary sent: I'm alive!\n");
			usleep(1);
		}		
		sendBroadcast(counter);
		printf("Primary sent: %d\n", counter);
		usleep(400000);
		counter++;
	}
}

	
void backupFunc(char *newBackupCommand){
	printf("Starting backup.\n");
	int recSock;
	int myPort = 20011;
	struct sockaddr_in remaddr;
	socklen_t remaddrLen = sizeof(remaddr);
	struct sockaddr_in myaddr = {.sin_family = AF_INET, .sin_port = htons(myPort), .sin_addr.s_addr = htonl(INADDR_ANY)};
	socklen_t myaddrLen = sizeof(myaddr);
	int socketPermission = 1;

	struct timeval timeout = {.tv_sec = 4, .tv_usec = 0};
	fd_set readfds;
	long backupCount = 1;
	
	if ((recSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Failed to create recSocket.\n");
	}
	if (setsockopt(recSock, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
		perror("sendSocket reuse-port enable failed");
	}
	if (bind(recSock, (struct sockaddr *)&myaddr, myaddrLen) < 0){
		perror("Failed to bind recSocket.\n");
	}
	
	while(1){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock+1, &readfds, 0, 0, &timeout);
		switch (event){
			case -1:
				printf("Switch/select failed.\n");
				break;
			case 0:
				printf("Timed out, backup becomes primary.\n");
				close(recSock);
				//system(newBackupCommand);
				primaryFunc(backupCount+1);
				break;
			default:
				recvfrom(recSock, bufMessage, BUF_SIZE, 0, (struct sockaddr *)&remaddr, &remaddrLen);
				if (!strcmp(bufMessage, "I'm alive!\n")){
					printf("Backup received: I'm alive!\n");
					timeout.tv_sec = 4;
					timeout.tv_usec = 0;
				}else{
					backupCount = strtol(bufMessage, NULL, 0);
					printf("Backed up: %lu\n", backupCount);
				}
		}
					
		
	}
	
}


void main(int argc, char**argv){
	/*
	char *startOption;
	if (argc == 1){
		startOption = "primary";	
	}else if (argc == 2){
		startOption = argv[1];
	}else{
		printf("Too many arguments\n");
		return;
	}
	*/
	char newBackupCommand[200];
	sprintf(newBackupCommand, "gnome-terminal -e \"%s backup\"", argv[0]);
	backupFunc(newBackupCommand);
	/*
	if (!strcmp(startOption, "primary")){
		int startValue = 1;
		primaryFunc(startValue);
		system(newBackupCommand);
	}else if (!strcmp(startOption, "backup")){
		backupFunc(newBackupCommand);
	}
	printf("This should never happen.\n");
	*/
	return;
}







