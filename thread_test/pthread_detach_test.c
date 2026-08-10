#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>

void *worker(void *args){
    int i = 0;
    while(1){
        printf("Worker running…… %d\n",i++);
        sleep(1);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    
    pthread_t pid;
    pthread_create(&pid,NULL,worker,NULL);
    pthread_detach(pid);
    printf("Main running......\n");
    sleep(60);
    return 0;
}
