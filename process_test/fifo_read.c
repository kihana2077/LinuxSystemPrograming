#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#define MAX_SIZE 50
int main(int argc, char const *argv[])
{
    
    int fd;
    char *pipe_path = "/tmp/myfifo";
    // if(mkfifo(pipe_path,0664) != 0){
    //     perror("mkfifo");
    //     exit(EXIT_FAILURE);
    // }

    fd = open(pipe_path,O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    char buf[MAX_SIZE];
    size_t read_char;
    while(read_char = (read(fd,buf,strlen(buf))) > 0){
        write(STDOUT_FILENO,buf,strlen(buf));
    }

    if(read_char < 0){
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("读取数据完成\n");

    return 0;
}
