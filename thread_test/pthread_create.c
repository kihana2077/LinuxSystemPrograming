#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>

#define BUF_LEN 1024
char *BUF;
int flag = 0;
int running = 1;
void *thread_input(void *arg){
    while(1){
        while(flag == 0){
            int n = read(STDIN_FILENO,BUF,BUF_LEN-1);
            BUF[n] = '\0';
            flag = 1;
            if(n == 0){
                running = 0;
                printf("input線程正在退出\n");
                pthread_exit(NULL);
            }

        }
    }
        return NULL;
}

void *thread_echo(void *argv){
    while(1){
        if(!running && flag){
            printf("echo線程正在退出\n");
            pthread_exit(NULL);
        }
        while(flag == 1){
            fprintf(stdout,"Echo::%s",BUF);
            flag = 0;
        }
    }
    return NULL;
}
int main(int argc, char const *argv[])
{
    BUF = (char *)malloc(BUF_LEN);
    memset(BUF,0,BUF_LEN);
    pthread_t input,output;

    pthread_create(&input,NULL,thread_input,NULL);
    pthread_create(&output,NULL,thread_echo,NULL);

    pthread_join(input,NULL);
    pthread_join(output,NULL);
    printf("程序正在退出……\n");
    free(BUF);
    return 0;
}
