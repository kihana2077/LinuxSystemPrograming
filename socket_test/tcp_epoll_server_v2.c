#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<stdbool.h>
#define MAX_CONNECTIONS 16384
#define MAX_EVENTS 8192
#define LISTEN_BACKLOG 8192
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
    bool should_close;
}Client;


void _init_client(Client *c){
        c->client_fd = -1;
        c->used = 0;
        c->send_len = 0;
        c->send_offset = 0;
        c->should_close = false;
        memset(c->recv_buf,0,sizeof(c->recv_buf));
        memset(c->recv_message,0,sizeof(c->recv_message));
        memset(c->send_buf,0,sizeof(c->send_buf));
}

int handle_recv(int epfd,struct epoll_event *ev){
    Client *c = ev->data.ptr;
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
        return 1;
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
    if(c->send_len > c->send_offset){
        ev->events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        epoll_ctl(epfd,EPOLL_CTL_MOD,c->client_fd,ev);
    }
    return 0;
}

int handle_send(int epfd,struct epoll_event *ev){
    Client *c = ev->data.ptr;
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
            ev->events = EPOLLIN | EPOLLRDHUP;
            epoll_ctl(epfd,EPOLL_CTL_MOD,c->client_fd,ev);
            c->send_len = 0;
            c->send_offset = 0;
            return 1;
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
    int conns = 0;
    int epfd = epoll_create1(0);
    if(epfd == -1){
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if(set_nonblock(server_fd) == -1){
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;
    epoll_ctl(
        epfd,
        EPOLL_CTL_ADD,
        server_fd,
        &ev
    );
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9191);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt = 1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(bind(server_fd,(struct sockaddr *)&addr,sizeof(addr)) == -1){
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,LISTEN_BACKLOG) == -1){
        perror("listen");
        close(server_fd);
        exit(1);
    }
    printf("Listening...\n");

    struct epoll_event evs[MAX_EVENTS + 1];
    while(1){
    int n = epoll_wait(epfd,evs,MAX_EVENTS + 1,-1);
        for(int i = 0;i < n;i++ ){
            uint32_t revents = evs[i].events;
            if(evs[i].data.ptr == NULL && (revents & EPOLLIN)){
                int cli_fd = accept(server_fd,NULL,NULL);
                if(cli_fd == -1){
                    perror("accept");
                    continue;
                }
                if(set_nonblock(cli_fd) == -1){
                    close(cli_fd);
                    continue;
                }
                conns++;
                if(conns > MAX_CONNECTIONS){
                    //背压，尚未实现
                    close(cli_fd);
                    conns--;
                    continue;
                }
                struct epoll_event evc;
                evc.events = EPOLLIN | EPOLLRDHUP;
                Client *client = malloc(sizeof(*client));
                if(client == NULL){
                    conns--;
                    perror("malloc");
                    close(cli_fd);
                    continue;
                }
                _init_client(client);
                client->client_fd = cli_fd;
                evc.data.ptr = client;
                
                int ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cli_fd,&evc);
                if(ret == -1){
                    conns--;
                    close(cli_fd);
                    free(client);
                    continue;
                }
                printf("One client has connected\n");
                continue;
            }
            Client *c = evs[i].data.ptr;
            if(revents & EPOLLIN){
                //recv
                int ret = handle_recv(epfd,&evs[i]);

                if(ret == -1){
                    c->should_close = true;
                }
                if((revents & EPOLLRDHUP) && ret){
                        c->should_close = true;
                }
                
            }
            if(revents & EPOLLOUT){
                //send
                int ret = handle_send(epfd,&evs[i]);
                if(ret == -1){
                    epoll_ctl(epfd,EPOLL_CTL_DEL,c->client_fd,NULL);
                    close(c->client_fd);
                    free(c);
                    continue;
                }else if(ret == 1){
                if(c->should_close){
                    conns--;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,c->client_fd,NULL);
                    close(c->client_fd);
                    free(c);
                    continue;
                }
                }
            }
        }
    }
    return 0;
}
