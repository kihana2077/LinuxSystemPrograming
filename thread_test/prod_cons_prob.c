#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
//旨在构建一个最小的生产者消费者模型，以便理解服务器接收请求之后的任务分发策略
int data;
int ready = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full   = PTHREAD_COND_INITIALIZER;

void *producer(void *arg){
    for(int i = 0;i<10;i++){
        pthread_mutex_lock(&mutex);
        while(ready == 1){
            pthread_cond_wait(&not_full,&mutex);
        }
        data = i+1;
        ready = 1;
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
        printf("Producer:: %d\n",data);
    }
    return NULL;
}

void *consumer(void *arg){
    for(int i = 0;i<10;i++){
        pthread_mutex_lock(&mutex);
        while(ready == 0){
            pthread_cond_wait(&not_empty,&mutex);
        }
        ready = 0;
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);
        printf("Consumer:: %d\n",data);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    pthread_t prod,cons;

    pthread_create(&prod,NULL,producer,NULL);
    pthread_create(&cons,NULL,consumer,NULL);

    pthread_join(prod,NULL);
    pthread_join(cons,NULL);

    
    return 0;
}


