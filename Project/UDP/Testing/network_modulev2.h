#ifndef _network_modulev2_
#define _network_modulev2_

#include <stdio.h>
#include <pthread.h>

struct bufferInfo;

int init_network();
void* send_message(void *args);
void* listen_for_messages(void *args);
struct bufferInfo decodeMessage(char *buffer);
void encodeMessage(char *buffer, struct bufferInfo information);

#endif