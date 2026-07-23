#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>
int main(int argc, char const *argv[])
{
    
    pid_t pid;
    if(argc != 2){
        fprintf(stderr,"请输入参数");
        exit(EXIT_FAILURE);
    }


    //父进程传输数据，子进程通过匿名管道读并且打印到控制台上‘’
    int pipefd[2];
    if(pipe(pipefd) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){
        close(pipefd[1]);
        char buf;
        printf("Son process is now reading information\n");
        while(read(pipefd[0],&buf,1) > 0){
            write(STDOUT_FILENO,&buf,1);
        }
        printf("\n");
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
        
    }else{
        close(pipefd[0]);//读端保护
        printf("Now parent %d process is using unnamed pipe to communicate:\n",getpid());
        write(pipefd[1],argv[1],strlen(argv[1]));
        close(pipefd[1]);
        waitpid(pid,NULL,0);

    }
    return 0;
}
