#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

int main(int argc, char const *argv[])
{
    
    pid_t pid = fork();
    if(pid < 0){
        perror("fork failure:");
    }
    if(pid == 0 ){
        printf("First get PPID is:%d\n",getppid());
        sleep(3);
        printf("Awake and get PID again:%d\n",getpid());
        printf("Now PPID is:%d\n",getppid());
    }else{
        printf("This is parent %d and create a son %d \n",getpid(),pid);
        sleep(1);
        printf("parent process is exiting!\n");
        exit(0);
    }
    return 0;
}
