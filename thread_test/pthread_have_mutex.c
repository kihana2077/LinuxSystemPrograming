#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

int count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *th1(void *args){
    int i = 0;
    pthread_mutex_lock(&mutex);
    while(i < 100000){
        count++;
        i++;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *th2(void *args){
    int i = 0;
    pthread_mutex_lock(&mutex);
    while(i < 100000){
        count++;
        i++;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    pthread_t pid1,pid2;
    // pthread_mutex_t mutex;
    // pthread_mutex_init(&mutex,NULL);

    pthread_create(&pid1,NULL,th1,NULL);
    pthread_create(&pid2,NULL,th2,NULL);

    pthread_join(pid1,NULL);
    pthread_join(pid2,NULL);

    printf("value is : %d\n",count);
    return 0;
}
