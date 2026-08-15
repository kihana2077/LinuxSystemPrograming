#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char const *argv[])
{
    int client_fd = socket(AF_INET,SOCK_STREAM,0);
    if(client_fd == -1){
        perror("socket");
        exit(1);
    }
    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(9191);
    
    if (inet_pton(AF_INET,
                  "127.0.0.1",
                  &target_addr.sin_addr) != 1) {

        perror("inet_pton");
        close(client_fd);
        return 1;
    }
    if(connect(client_fd,(struct sockaddr*)&target_addr,sizeof(target_addr)) == -1){
        perror("connect");
        close(client_fd);
        exit(1);
    }
    printf("Server connected!\n");
    char *message = "Hello from client";
    send(client_fd,message,strlen(message),0);
    
    char buf[1024];
    ssize_t n = recv(client_fd,buf,sizeof(buf)-1,0);
    if(n == 0){
        perror("recv");
    }
    if(n > 0){
        buf[n] = '\0';
        printf("Received:: %s\n",buf);

    }
    sleep(3);
    close(client_fd);

    return 0;
}
