#ifndef _network_module_
#define _network_module_

#include <stdio.h>
#include <pthread.h>


int init_network();
void* send_message(void *args);
void* listen_for_messages(void *args);

#endif