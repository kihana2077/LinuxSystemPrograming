#include<mqueue.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>

#define MY_QUEUE "/my_mq"

int main(int argc, char const *argv[])
{
    /*
    struct mq_attr{}消息队列的属性信息
    mq_flags,标记，对于mqopen忽略
    mq_maxmgs,最大消息数量
    mq_msgsize,单条消息最大字节数
    mq_curmsgs当前消息条数，对于mqopen忽略
    */
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 128;
    mqd_t mqdes = mq_open(MY_QUEUE,O_CREAT | O_RDWR | O_EXCL,0664,&attr);//创建消息队列

    if(mqdes == (mqd_t)-1){
        perror("mq_open");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        exit(1);
    }

    if(pid == 0){
        char b_buf[128];
        for(size_t i = 0;i<attr.mq_maxmsg;i++){
            memset(b_buf,0,sizeof(b_buf));
            if(mq_receive(mqdes,b_buf,sizeof(b_buf),NULL) == -1){
                perror("mq_receive");
                exit(1);
            }
            printf("子进程接收到数据：%s",b_buf);
        }
        

    }else{
        //父进程通过队列发送消息给子进程
        char a_buf[128];
        for(size_t i = 0;i < attr.mq_maxmsg;i++){
            memset(a_buf,0,sizeof(a_buf));
            sprintf(a_buf,"这是第%d条消息\n",(i + 1));
            if(mq_send(mqdes,a_buf,strlen(a_buf) + 1,0) == -1){
                perror("mq_send"); 
            }
            printf("父进程发送了一条消息\n");
            sleep(1);
        }
        waitpid(pid,NULL,0);
        

    }

    mq_close(mqdes);
    if(pid == 0){
        exit(0);
    }

    if(pid > 0){
        mq_unlink(MY_QUEUE);
    }

    return 0;
}