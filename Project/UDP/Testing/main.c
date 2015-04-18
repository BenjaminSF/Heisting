#include "network_modulev2.h"
#include <stdio.h>

int main(){
	int masterStatus = init_network();
    if (masterStatus == -1){
        printf("Network initialization failed");
    }
    return 0;
}