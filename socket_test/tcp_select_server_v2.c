#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include<fcntl.h>
#define MAX_CONNECTIONS 256

int set_nonblock(int fd){
    int flags = fcntl(fd,F_GETFL,0);
    if(flags == -1){
        perror("fcntl");
        return -1;
    }
    return fcntl(fd,F_SETFL, flags | O_NONBLOCK);
}

typedef struct Client{
    int client_fd;
    //接收缓冲区
    char recv_buf[1024];
    char recv_message[4096];
    size_t used;        //recv_message中尚未有多少字节被解析掉
    //发送缓冲区
    char send_buf[4096];
    size_t send_len;    //send_buf中目前有多少字节是等待发送数据
    size_t send_offset; //send_buf已经发送到哪个位置
}Client;

void _init_client(Client c[],int num){
    for(int i = 0;i < num; i++){
        c[i].client_fd = -1;
        c[i].used = 0;
        c[i].send_len = 0;
        c[i].send_offset = 0;
        memset(c[i].recv_buf,0,sizeof(c[i].recv_buf));
        memset(c[i].recv_message,0,sizeof(c[i].recv_message));
        memset(c[i].send_buf,0,sizeof(c[i].send_buf));
    }
}

int alloc_client(Client c[],int fd){
    for(int slot = 0;slot < MAX_CONNECTIONS;slot++){
        if(c[slot].client_fd == -1){
            c[slot].client_fd = fd;
            return slot;
        }
    }
    return -1;
}

int handle_recv(Client *c){
    ssize_t n = recv(c->client_fd,c->recv_buf,sizeof(c->recv_buf),0);



    if(n > 0){
        if(c->used + (size_t)n > sizeof(c->recv_message)){
            fprintf(stderr,"Message too long!\n");
            return -1;
        }
        memcpy(c->recv_message + c->used,c->recv_buf,n);
        c->used += n;
    }else if(n == 0){
        printf("Client disconnected.\n");
        return -1;
    }else{
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return 0;
        }
        perror("recv");
        return -1;
    }

    while(1){
        ssize_t msg_len = -1;
        for(int i = 0;i < c->used;i++){
            if(c->recv_message[i] == '\n'){
                msg_len = i+1;
                break;
            }
        }
        if(msg_len == -1){
            break;
        }

        if(c->send_len+ (size_t)msg_len >sizeof(c->send_buf)){
            fprintf(stderr,"Send_buf is full!\n");
            return -1;
        }
        memcpy(
            c->send_buf + c->send_len,
            c->recv_message,
            msg_len
        );
        c->send_len += msg_len;

        memmove(
            c->recv_message,
            c->recv_message + msg_len,
            c->used - msg_len
        );
        c->used -= msg_len;
    }
    return 0;
}

int handle_send(Client *c){
    if(c->send_len == c->send_offset){
        return 0;
    }
    ssize_t n = send(
        c->client_fd,
        c->send_buf + c->send_offset,
        c->send_len - c->send_offset,
        MSG_NOSIGNAL
    );
    if(n > 0){
        c->send_offset += n;
        if(c->send_len == c->send_offset){
            c->send_len = 0;
            c->send_offset = 0;
        }
        return 0;
    }else if(n == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return 0;
        }
        if(errno == EINTR){
            return 0;
        }
        perror("send");
        return -1;
    }else{
        perror("send");
        return -1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    Client client[MAX_CONNECTIONS];
    _init_client(client,MAX_CONNECTIONS);
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    set_nonblock(server_fd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9191);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt = 1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(bind(server_fd,(struct sockaddr *)&addr,sizeof(addr)) == -1){
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if(listen(server_fd,MAX_CONNECTIONS) == -1){
        perror("listen");
        close(server_fd);
        exit(1);
    }
    printf("Listening...\n");

    fd_set readfds;
    fd_set writefds;
    fd_set master_read;
    fd_set master_write;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&master_read);
    FD_ZERO(&master_write);
    FD_SET(server_fd,&readfds);
    master_read = readfds;
    master_write = writefds;
    int max_fd = server_fd;
    while(1){
        int ready = select(max_fd + 1,&readfds,&writefds,NULL,NULL);
        if(ready == -1){
            if(errno == EINTR){
                continue;
            }
            perror("select");
            break;
        }


        if(FD_ISSET(server_fd,&readfds)){
            int temp_fd = accept(server_fd,NULL,NULL);
            if(temp_fd == -1){
                if(errno == EAGAIN || errno == EWOULDBLOCK){

                }else if(errno == EINTR){

                }else{
                    perror("accept");
                }
            }else{
                if(set_nonblock(temp_fd) == -1){
                    close(temp_fd);
                }else{
                    int slot = alloc_client(client,temp_fd);
                    if(slot == -1){
                    printf("Max connections,please try again later...\n");
                    close(temp_fd);
                    //此处可做背压
                    }else{
                        if(temp_fd > max_fd){
                            max_fd = temp_fd;
                        }
                        printf("One client has connect.\n");
                        FD_SET(client[slot].client_fd,&master_read);
                    }
                }
            }
        }


        for(int i = 0;i < MAX_CONNECTIONS;i++){
            if(client[i].client_fd == -1){
                continue;
            }
            if(FD_ISSET(client[i].client_fd,&readfds)){
                    int flag = handle_recv(&client[i]);
                    if(client[i].send_offset < client[i].send_len){
                        FD_SET(client[i].client_fd,&master_write);
                    }
                    if(flag == -1){
                        FD_CLR(client[i].client_fd, &master_read);
                        FD_CLR(client[i].client_fd, &master_write);

                        close(client[i].client_fd);

                        client[i].client_fd = -1;
                        client[i].used = 0;
                        client[i].send_len = 0;
                        client[i].send_offset = 0;

                        continue;
                    }
            }
            if(FD_ISSET(client[i].client_fd,&writefds)){
                int tag = handle_send(&client[i]);
                if(tag == -1){
                    FD_CLR(client[i].client_fd, &master_read);
                    FD_CLR(client[i].client_fd, &master_write);

                    close(client[i].client_fd);

                    client[i].client_fd = -1;
                    client[i].used = 0;
                    client[i].send_len = 0;
                    client[i].send_offset = 0;
                    continue;
                }
                if(client[i].send_len == client[i].send_offset){
                    FD_CLR(client[i].client_fd,&master_write);
                }
            }
        }
        readfds = master_read;
        writefds = master_write;
    }
    return 0;
}
