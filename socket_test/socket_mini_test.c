#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
int main(void)
{
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);


    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        perror("socket");
        return 1;
    }

    printf("socket fd = %d\n",fd);
    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr)) == -1){
        perror("bind");
        return 1;
    }
    printf("Bind success\n");

    close(fd);
    return 0;
}
