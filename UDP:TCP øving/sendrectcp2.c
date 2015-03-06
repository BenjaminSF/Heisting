#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <errno.h>
void* client_func(){
	char *serverIP = "129.241.187.136";
	struct addrinfo cHints, *cRes;
	int cSocketfd;
	char* serverPort = "33546";
	char buf[1024];
	char msg[1024] = "Connect to: 129.241.187.161:20008";

	memset(&cHints, 0, sizeof(cHints));
	cHints.ai_family = AF_INET;
	cHints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(serverIP, serverPort, &cHints, &cRes) < 0){
		perror("Failed to set addrinfo");
	}
	if ((cSocketfd = socket(cRes->ai_family, cRes->ai_socktype, cRes->ai_protocol)) < 0){
		perror("Failed to init socket");
	}
	if (connect(cSocketfd, cRes->ai_addr, cRes->ai_addrlen) < 0){
		perror("Failed to connect");
	}
	if (recv(cSocketfd, buf, sizeof(buf), 0) < 0){
		perror("Failed to receive message");
	}
	printf("Client: Message received: %s \n", buf);
	if (send(cSocketfd, msg, sizeof(msg), 0) < 0){
		perror("Failed to send message");
	}
	close(cSocketfd);
	
}
void* server_func(){
	struct sockaddr_storage rem_addr;
	struct addrinfo sHints, *sRes;
	int sSocketfd, new_sock, status;
	char buf[10240];
	char *myPort = "20008";
	socklen_t addr_size;
	int yes = 1;

	memset(&sHints, 0, sizeof(sHints));
	sHints.ai_family = AF_INET;
	sHints.ai_socktype = SOCK_STREAM;
	sHints.ai_flags = AI_PASSIVE;
	
	if (getaddrinfo(NULL, myPort, &sHints, &sRes) < 0){
		perror("Failed to set addrinfo, server");
	}
	if ((sSocketfd = socket(sRes->ai_family, sRes->ai_socktype, sRes->ai_protocol)) < 0){
		perror("Failed to init socket, server");
	}
	if (setsockopt(sSocketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0){
		perror("Failed to set socket options");
	}	

	if (bind(sSocketfd, sRes->ai_addr, sRes->ai_addrlen) < 0){
		perror("Failed to bind socket");
	}
	listen(sSocketfd, 5);
	addr_size = sizeof(rem_addr);	
	char msg[1024] = "Hallo!\0";
	//msg[10] = '\0';

	while(1){

		if((new_sock = accept(sSocketfd, (struct sockaddr *)&rem_addr, &addr_size))<0){
			perror("Failed to accept");	
		}
		printf("Connection established\n");
		while(1){
			if((status = recv(new_sock,buf, 10240,0)) < 0){
				perror("Failed to recieve welcome message");
			}else if(status == 0){
				perror("Connection closed");
			}
			
			buf[status] = '\0';
			printf("Server: Message received: %s\n", buf);
			
			if (send(new_sock,msg, 1024, 0) < 0){
				perror("Failed to send test message");
			}
			printf("Sending message...");
			sleep(1);
			//printf("Server: Message received: %s\n", buf);
			memset(&buf[0],'\0',sizeof(buf));
		}
		//sleep(1);
		close(new_sock);
		memset(&buf[0],'\0',sizeof(buf));
		
	}
	//close(new_sock);
	close(sSocketfd);
}
void main(){
	pthread_t server_t, client_t;
	pthread_create(&server_t, NULL, server_func, NULL);
	pthread_create(&client_t, NULL, client_func, NULL);
	pthread_join(server_t, NULL);
	pthread_join(client_t, NULL);	
}
