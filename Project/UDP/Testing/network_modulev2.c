#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>


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
    struct lock rwLock;
    struct condition available;
    pthread_mutex_t rwLock;
    pthread_cond_t readReady;
};

enum bufferState{
	MSG_CONNECT,
	MSG_ELEVSTATE
};

struct bufferInfo{
	char *srcAddr;
	char *dstAddr;
	int masterStatus;
	enum bufferState myState;
};

void* send_message(void *args){
    time_t startTime, endTime;
    time(&startTime);

    return 0;
}


void *listen_for_messages(void *args){
    int recSock;
    struct sockaddr_in remaddr;
    socklen_t remaddrLen = sizeof(remaddr);
    struct sockaddr_in myaddr = {.sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY)};
    int socketPermission = 1;
    
    if ((recSock = socket(AF_INET,SOCK_DGRAM,0)) < 0){
    	perror("Failed to create recSocket.\n");
    }
    if (setsockopt(recSock, SOL_SOCKET, SO_REUSEPORT, (void*)&socketPermission, sizeof(socketPermission)) < 0){
    	perror("sendSocket reuse-port enable failed");
    }
    if (bind(recSock, (struct sockaddr *)&myaddr, myaddrLen) < 0){
		perror("Failed to bind recSocket.\n");
    }
    struct ListenParams *myArgs = (struct ListenParams*)args;
    printf("Test mottak, port: %d\n", (*myArgs).port);
    struct timeval timeout = {.tv_sec = 0, .tv_usec = (*myArgs).timeoutMs * 1000};
    fd_set readfds;
    //prepare socket
	while (1){
		FD_ZERO(&readfds);
		FD_SET(recSock, &readfds);
		int event = select(recSock, &readfds, 0, 0, &timeout);
		switch (event){
			case -1:
				printf("Switch/select for recSock failed.\n");
				break;
			case 0:
				close(recSock);
				break;
			default:
				pthread_mutex_lock((*myArgs.rwLock));
				
				recvfrom(recSock, bufMessage, BUF_SIZE, 0, (struct sockaddr *)&remaddr, &remaddrLen);
				pthread_cond_signal((*myArgs).available, (*myArgs).rwLock);
				pthread_mutex_unlock((*myArgs.rwLock));

    return 0;
}

int init_network(){
	//Finds the local machine's IP address
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
    info.port = 20008; //Set to a static value for port, could implement and call a function if necessary
    
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
    tmpIP[index+4] = '\0'gfg;
    info.broadcastIP = strdup(tmpIP);
    printf("Lokalt: IP: %s\t Port: %d\t Broadcast: %s\n", info.localIP, info.port, info.broadcastIP);
    
    //Start listening for responses
    struct ListenParams params = {.port = info.port};
    pthread_mutex_init(&(params.rwLock), NULL);
    pthread_cond_init(&(params.readReady), NULL);
    pthread_t findOtherElevs;
    pthread_create(&findOtherElevs, NULL, &listen_for_messages, &params);
    
    int addrslistCounter = 0;
    while(pthread_kill(&findOtherElevs, 0) != ESRCH){
	pthread_mutex_lock(&(params.rwLock));
	pthread_cond_wait(&(params.readReady), &(params.rwLock))
	struct bufferInfo bufInfo;
	bufInfo = decodeMessage(&bufMessage);
	if (bufInfo.msgState == MSG_CONNECT){ //Only use related messages
	
		info.addrslist[addrslistCounter] = bufInfo.srcAddr;
		addrslistCounter++;
		if (bufInfo.masterStatus == 1){
			//master not available
			//current master is bufInfo.srcAddr
		}
		//add buffer to address list
	}
	pthread_mutex_unlock(&(params.rwLock));
    }
    //Finished listening
    pthread_join(findOtherElevs, NULL);
    pthread_mutex_destroy(&(params.rwLock));
    pthread_cond_destroy(&(params.readReady));
    return 0;
}
