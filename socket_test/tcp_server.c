#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>

int main(int argc, char const *argv[])
{
    
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        perror("socket");
        exit(1);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9191);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(fd,(struct sockaddr*)&server_addr,sizeof(server_addr)) == -1){
        perror("bind");
        close(fd);
        exit(1);
    }

    if(listen(fd,10) == -1){
        perror("listen");
        close(fd);
        exit(1);
    }
    printf("Waiting for connect...\n");

    int client_fd = accept(fd,NULL,NULL);
    if(client_fd == -1){
        perror("accept");
        close(fd);
        exit(1);
    }
    printf("Client connected!\n");

    char buf[1024];
    ssize_t n = recv(client_fd,buf,sizeof(buf)-1,0);
    if(n <= 0){
        printf("error\n");
    }
    if(n > 0){
        buf[n] = '\0';
        printf("Received:: %s\n",buf);
        char *response = "Hello from server";
        send(client_fd,response,strlen(response),0);
    }
    sleep(3);
    close(client_fd);
    close(fd);

    return 0;
}
