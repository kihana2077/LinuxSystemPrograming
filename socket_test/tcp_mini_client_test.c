#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(void)
{
    int client_fd = socket(AF_INET,SOCK_STREAM,0);
    if(client_fd == -1){
        perror("socket");
        close(client_fd);
        return 1;
    }

    struct sockaddr_in targetaddr;
    inet_pton(AF_INET,"127.0.0.1",&targetaddr.sin_addr);
    targetaddr.sin_family = AF_INET;
    targetaddr.sin_port = htons(1145);
    
    if(connect(client_fd,(struct sockaddr*)&targetaddr,sizeof(targetaddr)) == -1){
        perror("connect");
        close(client_fd);
        return 1;
    }
    printf("Connect success!!\n");
    close(client_fd);
    return 0;
}
