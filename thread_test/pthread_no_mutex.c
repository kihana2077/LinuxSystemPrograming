#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

int count = 0;

void *th1(void *args){
    int i = 0;
    while(i < 100000){
        count++;
        i++;
    }
    return NULL;
}

void *th2(void *args){
    int i = 0;
    while(i < 100000){
        count++;
        i++;
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    pthread_t pid1,pid2;
    pthread_create(&pid1,NULL,th1,NULL);
    pthread_create(&pid2,NULL,th2,NULL);

    pthread_join(pid1,NULL);
    pthread_join(pid2,NULL);

    printf("value is : %d\n",count);//此时结果尚未达到200000,说明线程之间发生了竞态
    return 0;
}
