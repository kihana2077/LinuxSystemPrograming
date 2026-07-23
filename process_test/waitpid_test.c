#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
int main(int argc, char const *argv[])
{
    printf("This is a parent process,PID is: %d\n",getpid());
    pid_t pid = fork();
    if(pid == 0){
        sleep(2);
        exit(EXIT_SUCCESS);
    }else{
        int flag = 0;
        while(flag == 0)
        {
            flag = waitpid(pid,NULL,WNOHANG);
            if(flag == 0){
                printf("Not recycle yet.\n");
                sleep(1);
            }else{
                printf("Recycle ready.\n");
            }
        }
    }
    return 0;
}
