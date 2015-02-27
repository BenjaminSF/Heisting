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
    pthread_mutex_t timeoutLock;
};

void* send_message(void *args){
    return 0;
}


void *listen_for_messages(void *args){
    struct ListenParams *myArgs = (struct ListenParams*)args;
    printf("Test mottak, port: %d\n", (*myArgs).port);
    while (pthread_mutex_trylock((*myArgs).timeoutLock) == EBUSY){
        printf("Testing...\n");
        usleep(50);
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
    tmpIP[index+4] = '\0';
    info.broadcastIP = strdup(tmpIP);
    printf("Lokalt: IP: %s\t Port: %d\t Broadcast: %s\n", info.localIP, info.port, info.broadcastIP);
    
    //Start listening for responses
    struct ListenParams params = {.port = info.port};
    pthread_mutex_init(&(params.timeoutLock), NULL);
    pthread_mutex_lock(&(params.timeoutLock));
    pthread_t findOtherElevs;
    pthread_create(&findOtherElevs, NULL, &listen_for_messages, &params);
    //Finished listening
    usleep(300);
    pthread_mutex_unlock(&(params.timeoutLock));
    pthread_join(findOtherElevs, NULL);
    return 0;
}
