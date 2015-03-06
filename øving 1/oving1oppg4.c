
#include <pthread.h>
#include <stdio.h>
int i = 0;
pthread_mutex_t mutex;

void* thread_1func(void *tempInput){
int k;

for (k = 0; k< 1000000;k++){
	pthread_mutex_lock(&mutex);	
	i++;	
	pthread_mutex_unlock(&mutex);
}

return NULL;
}

void* thread_2func(void *tempInput){

int k;
for (k= 1000000; k>0;k--){
	pthread_mutex_lock(&mutex);	
	i--;
	pthread_mutex_unlock(&mutex);
}

return NULL;
}

int main(){
	pthread_mutex_init(&mutex, NULL);
	pthread_t thread_1,thread_2;
	pthread_create(&thread_1,NULL,thread_1func,NULL);
	pthread_create(&thread_2,NULL,thread_2func,NULL);
	int j = 0;	
	while(j < 100)
	{
		usleep(1);
		++j;
	}
	pthread_join(thread_1,NULL);
	pthread_join(thread_2,NULL);
	printf("%d\n", i);
	pthread_mutex_destroy(&mutex);
	return 0;
}

