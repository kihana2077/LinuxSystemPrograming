#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
int main(int argc, char const *argv[])
{
    char *name = "parent";
    printf("This is %s Process,PID is %d\n",name,getpid());
    pid_t pid = fork();
    //int execve(const char *__path执行程序的路径, char *const __argv[]传入的第二个参数,
    // char *const __envp[])环境变量
    char *args[] = {
        "ping",
        "-c",
        "10",
        "baidu.com",
        NULL
    };
    char *envs[] = {
        NULL
    };
    if(pid == 0){
        execve("/bin/ping",args,envs);
    }else{
        printf("parent process is waiting son procees finished\n");
        pid = wait(NULL);
        fflush(stdout);
        printf("process %d is finished,application will exit\n",pid);
    }
    return 0;
}
