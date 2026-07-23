#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    int fd = open("fork_test.txt",O_CREAT | O_WRONLY | O_APPEND,0644);

    if(fd == -1){
        perror("OPEN:");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if(pid < 0){
        perror("FORK:");
        exit(EXIT_FAILURE);
    }
    return 0;
}
