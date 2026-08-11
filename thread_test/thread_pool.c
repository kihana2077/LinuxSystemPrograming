//使用C语言手写线程池，从模拟面向对象到结构体撰写构造函数666
#define _GNU_SOURCE
#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#define MAX_TASK 100

typedef struct Task
{
    void (*function)(void *);
    void *arg;
}Task;

typedef struct TaskQueue{
    Task queue[MAX_TASK];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int shutdown;
}Queue;

void queue_init(Queue *q){
    q->front = 0;
    q->rear  = 0;
    q->size  = 0;
    q->shutdown = 0;
    pthread_mutex_init(&q->mutex,NULL);
    pthread_cond_init(&q->cond,NULL);
}

int push(Queue *q,Task task){
    pthread_mutex_lock(&q->mutex);
    if(q->size == MAX_TASK){
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    q->queue[q->rear] = task;
    q->rear = (q->rear + 1) % MAX_TASK;
    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

Task pop(Queue *q){
    Task task;
    pthread_mutex_lock(&q->mutex);
    while(q->size == 0){
        pthread_cond_wait(&q->cond,&q->mutex);
    }
    task = q->queue[q->front];
    q->front = (q->front +1)% MAX_TASK;
    q->size--;
    pthread_mutex_unlock(&q->mutex);
    return task;
}

void *worker(void *arg){
    Queue *q = (Queue *)arg;
    while(1){
        Task task;
        task = pop(q);
        task.function(task.arg);
        sleep(10);
    }
}

typedef struct ThreadPool{
    pthread_t threads[10];  
    Queue q;
}ThreadPool;

void threadpool_init(ThreadPool *pool){
    queue_init(&pool->q);
    for(int i = 0;i < 3;i++){
        pthread_create(&pool->threads[i],NULL,worker,&pool->q);
    }
}


void print_task(void *arg){
    int num = *(int *)arg;
    printf("The task number is:: %d\n",num);
}

int main(int argc, char const *argv[])
{
    ThreadPool pool;
    threadpool_init(&pool);
    Task print;
    print.function = print_task;
    int args[MAX_TASK];
    for(int i = 0;i< MAX_TASK;i++){
        args[i] = i;
        print.arg = &args[i];
        if(push(&pool.q,print) == -1){
            printf("任务队列已满，任务提交失败！\n");
        }
        usleep(500);
    }

    for(int i = 0; i < 3; i++){
        pthread_join(pool.threads[i], NULL);
    }
    return 0;
}
