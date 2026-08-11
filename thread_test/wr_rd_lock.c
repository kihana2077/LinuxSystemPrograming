#define _GNU_SOURCE
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<pthread.h>

int config = 100;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void *th_reader(void *arg){
    int id = *(int *)arg;
    sleep(1);
    pthread_rwlock_rdlock(&rwlock);
    printf("Reader:: %d has read the config is %d\n",id,config);
    pthread_rwlock_unlock(&rwlock);
    return NULL;
}

void *th_writer(void *arg){
    for(int i = 0;i< 5;i++){
        pthread_rwlock_wrlock(&rwlock);
        config += 10;
        printf("Writer update the config:: %d\n",config);
        pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t reader[3];
    pthread_t writer;

    int id[3] = {0 , 1 , 2};
        
    pthread_create(&writer,NULL,th_writer,NULL);
    for(int i = 0;i < 3;i++){
        pthread_create(&reader[i],NULL,th_reader,&id[i]);
    }


    for(int i = 0;i < 3;i++){
        pthread_join(reader[i],NULL);
    }
    pthread_join(writer,NULL);
    return 0;
}
