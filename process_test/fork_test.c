#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    printf("This is a parent process,called PID:%d\n",getpid());
    fflush(stdout);
    pid_t pid = fork();
    printf("%d ",pid);
    sleep(3);
    
    return 0;
}
