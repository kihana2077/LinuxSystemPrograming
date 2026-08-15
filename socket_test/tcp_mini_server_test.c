#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

int main(int argc, char const *argv[])
{
    
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd == -1){
        perror("socket");
        return 1;
    }
    int opt = 1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1145);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1){
        perror("bind");
        close(server_fd);
        return 1;
    }

    if(listen(server_fd,10) == -1){
        perror("LISTEN");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port 1145::\n");

    // struct sockaddr_in client_addr;
    // socklen_t client_len = sizeof(client_addr);
    
    int client_fd1 = accept(
        server_fd,
        NULL,
        NULL
    );
    if(client_fd1 == -1){
        perror("accept");
        close(client_fd1);
        close(server_fd);
        return 1;
    }

    printf("Client is connecting!! \n");

    close(client_fd1);
    // close(server_fd);
    sleep(10);
    return 0;
}
