
#include <pthread.h>
#include <stdio.h>

void* thread_1func(void *i){
int k, iTemp;
//iTemp = (int)i;
for (k = 0; k< 1000000;k++){
	//iTemp++;
	i++;	
}
return NULL;
}

void* thread_2func(void *i){
int k, iTemp;
//iTemp = (int)i;
for (k= 1000000; k>0;k--){
	//iTemp--;
	i--;
}
return NULL;
}

void main(){
	int i = 0;	
	pthread_t thread_1,thread_2;
	pthread_create(&thread_1,NULL,thread_1func,(void *)&i);
	pthread_create(&thread_2,NULL,thread_2func,(void *)&i);
	pthread_join(thread_1,NULL);
	pthread_join(thread_2,NULL);
	printf("%d\n", i);
}

