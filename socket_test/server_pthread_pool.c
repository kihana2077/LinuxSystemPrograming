#define _POSIX_C_SOURCE 202405L
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<signal.h>
#include<errno.h>
#include<pthread.h>
#include<fcntl.h>
#define MAX_MSG_LEN 4096
#define MAX_TASK 100
#define MAX_THREADS 10

typedef struct Task{
    int client_fd;
}Task;

typedef struct Queue{
    Task task_queue[MAX_TASK];
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
}Queue;

void queue_init(Queue *q){
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex,NULL);
    pthread_cond_init(&q->not_empty,NULL);
    pthread_cond_init(&q->not_full,NULL);
}

ssize_t send_all(int fd,const void *buf,size_t len){
    size_t n = 0;
    while(n < len){
        ssize_t sent = send(fd,(char *)buf + n,len - n,0);
        if(sent == -1){
            perror("send");
            return -1;
        }
        n +=sent;
    }
    return n;
}

void handle_client(void *arg){
    int client_fd = *(int *)arg;
    int flags = fcntl(client_fd,F_GETFL,0);
    if(flags == -1){
        perror("fcntl");
    }
    if(fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1){
    perror("fcntl");
    }
    
    char buf[1024];
    char message[4096];
    size_t used = 0;
    while(1){
        ssize_t n = recv(client_fd,buf,sizeof(buf)-1,0);
        if(n > 0){
            if(used + n > sizeof(message)){
                fprintf(stderr,"Message too long!\n");
                break;
            }
            memcpy(message + used,buf,n);
            used += n;
        }else if(n == 0){
            printf("Client disconnected.\n");
            break;
        }else{
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                printf("暂时没有数据\n");
            }else{
                perror("recv");
                break;
            }
        }
        
        while(1){
            ssize_t msg_len = -1;
            for(size_t i = 0;i< used;i++){
                if(message[i] == '\n'){
                    msg_len = i + 1;
                    break;
            }
        }
        if(msg_len == -1){
            break;
        }
        printf("Completly msg:: %.*s",(int)msg_len,message);
        fflush(stdout);
        send_all(client_fd,message,msg_len);
        memmove(message,message + msg_len,used - msg_len);
        used -= msg_len;
        }
    }
    close(client_fd);
}

void *worker(void *arg){
    Queue *q = (Queue *)arg;
    while(1){
        Task task;
        pthread_mutex_lock(&q->mutex);
        while(q->count == 0){
            pthread_cond_wait(&q->not_empty,&q->mutex);
        }
        task = q->task_queue[q->front];
        q->front = (q->front + 1) % MAX_TASK;
        q->count--;
        pthread_cond_signal(&q->not_full);
        pthread_mutex_unlock(&q->mutex);
        handle_client(&task.client_fd);
    }
    return NULL;
}

typedef struct ThreadPool{
    pthread_t threads[MAX_THREADS];
    Queue q;
}ThreadPool;


void threadpool_init(ThreadPool *pool){
    queue_init(&pool->q);
    for(int i = 0;i < MAX_THREADS;i++){
        if(pthread_create(&pool->threads[i],NULL,worker,&pool->q) != 0){
            perror("pthread_create");
            continue;
        }
        pthread_detach(pool->threads[i]);
    }
}

ssize_t welcome_client(int server_fd,Queue *q,size_t i){
    int client_fd = accept(server_fd,NULL,NULL);
    if(client_fd == -1){
        perror("accept");
        return -1;
    }
    pthread_mutex_lock(&q->mutex);
    while(q->count == MAX_TASK){
        pthread_cond_wait(&q->not_full,&q->mutex);
    }
    q->task_queue[q->rear].client_fd = client_fd;
    q->rear = (q->rear + 1) % MAX_TASK;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int main(int argc, char const *argv[])
{
    ThreadPool pool;
    threadpool_init(&pool);//线程池创建
    int server_fd = socket(AF_INET,SOCK_STREAM,0);//创建监控套接字

    int opt = 1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //设置取消TIME_WAIT

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9191);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_fd,(struct sockaddr *)&addr,sizeof(addr)) == -1){
        perror("bind");
        close(server_fd);
        exit(1);
    }
    
    if(listen(server_fd,1000) == -1){
        perror("listen");
        close(server_fd);
        exit(1);
    }else{
        printf("Waiting connection......\n");
    }

    // struct sockaddr_in c_addr; //client addr struct
    //accept
    size_t i = 0;
    while(1){
        welcome_client(server_fd,&pool.q,i);
        i++;
    }
    close(server_fd);
    return 0;
}

