#include<poll.h>
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

void _init_pollfd(struct pollfd fds[]){
    for(int i = 0;i < MAX_CONNECTIONS + 1;i++){
        fds[i].fd = -1;
        fds[i].events = 0;
    }
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

int handle_recv(Client c[],struct pollfd fds[],int i){
    ssize_t n = recv(c[i - 1].client_fd,c[i - 1].recv_buf,sizeof(c[i - 1].recv_buf),0);

    if(n > 0){
        if(c[i - 1].used + (size_t)n > sizeof(c[i - 1].recv_message)){
            fprintf(stderr,"Message too long!\n");
            return -1;
        }
        memcpy(c[i - 1].recv_message + c[i - 1].used,c[i - 1].recv_buf,n);
        c[i - 1].used += n;
    }else if(n == 0){
        printf("Client disconnected.\n");
        return 0;
    }else{
        perror("recv");
        return -1;
    }

    while(1){
        ssize_t msg_len = -1;
        for(int j = 0;j < c[i - 1].used;j++){
            if(c[i - 1].recv_message[j] == '\n'){
                msg_len = j+1;
                break;
            }
        }
        if(msg_len == -1){
            break;
        }

        if(c[i - 1].send_len+ (size_t)msg_len >sizeof(c[i - 1].send_buf)){
            fprintf(stderr,"Send_buf is full!\n");
            return -1;
        }
        memcpy(
            c[i - 1].send_buf + c[i - 1].send_len,
            c[i - 1].recv_message,
            msg_len
        );
        c[i - 1].send_len += msg_len;

        memmove(
            c[i - 1].recv_message,
            c[i - 1].recv_message + msg_len,
            c[i - 1].used - msg_len
        );
        c[i - 1].used -= msg_len;
    }
    if(c[i - 1].send_len > c[i - 1].send_offset){
        fds[i].events |= POLLOUT;
    }
    printf("recv is success\n");  //Debug log
    return 0;
}

int handle_send(Client c[],struct pollfd fds[],int i){
    if(c[i - 1].send_len == c[i - 1].send_offset){
        return 0;
    }
    ssize_t n = send(
        c[i - 1].client_fd,
        c[i - 1].send_buf + c[i - 1].send_offset,
        c[i - 1].send_len - c[i - 1].send_offset,
        MSG_NOSIGNAL
    );
    printf("send %ld char\n",n);
    printf("fragment is sent.\n");
    if(n > 0){
        c[i - 1].send_offset += n;
        if(c[i - 1].send_len == c[i - 1].send_offset){
            fds[i].events &= ~POLLOUT;
            c[i - 1].send_len = 0;
            c[i - 1].send_offset = 0;
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

int remove_conn(int i,struct pollfd fds[],Client c[],int active_conns){
    if(i < 0 || i > active_conns){
        return -1;
    }
    close(fds[i].fd);
    if(i != active_conns - 1){
        fds[i] = fds[active_conns - 1];
        c[i - 1] = c[active_conns - 2];
    }
    fds[active_conns].fd = -1;
    fds[active_conns].events = 0;
    c[active_conns - 1].client_fd = -1;
    c[active_conns - 1].used = 0;
    c[active_conns - 1].send_offset = 0;
    c[active_conns - 1].send_len = 0;
    return active_conns - 1;
}

int main(int argc, char const *argv[])
{
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
//设置监听socket

    struct pollfd fds[MAX_CONNECTIONS + 1];
    Client client[MAX_CONNECTIONS];
    _init_pollfd(fds);
    _init_client(client,MAX_CONNECTIONS);
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    int running = 1;
    int active_conns = 1;
    while(running){
        int ret = poll(fds,active_conns,-1);
        if(ret == -1){
            perror("poll");
            continue;
        }
        for(int i = 0;i < active_conns ;i++ ){
            if(fds[i].fd == server_fd && (fds[i].revents & POLLIN)){
                int client_fd = accept(fds[i].fd,NULL,NULL);
                if(client_fd == -1){
                    perror("accept");
                    continue;
                }
                set_nonblock(client_fd);
                printf("One client has connected.\n");
                fds[active_conns].fd = client_fd;
                fds[active_conns].events = POLLIN;
                client[active_conns - 1].client_fd = client_fd;
                active_conns++;
                continue;
            }
            if(fds[i].revents & POLLIN){
                //recv
                int ret = handle_recv(client,fds,i);
                if(ret == 0){
                    remove_conn(i,fds,client,active_conns);
                    i--;
                    continue;
                }
            }
            if(fds[i].revents & POLLOUT){
                //send
                handle_send(client,fds,i);
            }
            if(fds[i].revents & (POLLHUP | POLLERR)){
                active_conns = remove_conn(i,fds,client,active_conns);
                i--;
                continue;
            }
        }
    }
    return 0;
}


