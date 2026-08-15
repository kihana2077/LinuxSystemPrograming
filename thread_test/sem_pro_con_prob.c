#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<string.h>
#define BUF_SIZE 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t full;
sem_t empty;

// typedef struct Task{
//     void (*function)(void *);
//     void *arg;
// }task;

// void *square(void *arg){
//     int num = *(int *)arg;
//     printf(
//         "thread %lu: %d^2 = %d\n",
//         pthread_self(),
//         num,
//         num * num
//     );
// }

typedef struct Buffer{
    int data[BUF_SIZE];
    int front;
    int rear;
}buffer;

void buf_init(buffer *buf){
    memset(&buf->data,0,sizeof(buf->data));
    buf->front = 0;
    buf->rear = 0;
}

void produce(int i,buffer *q){
    pthread_mutex_lock(&mutex);
    q->data[i % BUF_SIZE] = i;
    pthread_mutex_unlock(&mutex);
    printf("Producer::Mission %d is created\n",i);
}

void consume(int i,buffer *q){
    pthread_mutex_lock(&mutex);
    printf("Consumer::Mission %d is accomplished\n",q->data[i % BUF_SIZE]);
    q->data[i % BUF_SIZE] = 0;
    pthread_mutex_unlock(&mutex);

}

void *producer(void *arg){
    buffer *buf = (buffer *)arg;
    for(int i = 0;i < 100;i++){
        sem_wait(&empty);
        produce(i,buf);
        sem_post(&full);
    }
    return NULL;
}

void *consumer(void *arg){
    buffer *buf = (buffer *)arg;
    for(int i = 0;i < 100;i++){
        sem_wait(&full);
        consume(i,buf);
        sem_post(&empty);
    }
}

void semaphore_init(void){
    sem_init(&full,0,0);
    sem_init(&empty,0,BUF_SIZE);
}

void semaphore_destory(void){
    sem_destroy(&full);
    sem_destroy(&empty);
}
int main(int argc, char const *argv[])
{
    buffer buf;
    buf_init(&buf);

    semaphore_init();
    pthread_t prod,cons;
    pthread_create(&prod,NULL,producer,&buf);
    pthread_create(&cons,NULL,consumer,&buf);

    pthread_join(prod,NULL);
    pthread_join(cons,NULL);
    semaphore_destory();
    return 0;
}
