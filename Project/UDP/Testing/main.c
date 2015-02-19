#include "network_module.h"
#include <stdio.h>

int main(){
    if (init_network() != 0){
        printf("Network initialization failed");
    }
    return 0;
}