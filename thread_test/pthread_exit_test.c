#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

void *worker(void *args){
    int i = 1;
    int *result = malloc(sizeof(int));
    *result = 0;
    while(i<=100){
        *result += i;
        i++;
    }
    pthread_exit(result);
}

int main(int argc, char const *argv[])
{
    pthread_t calc;

    pthread_create(&calc,NULL,worker,NULL);

    void *result;

    pthread_join(calc,&result);

    printf("结果是:%d\n", *(int *)result);

    return 0;
}
