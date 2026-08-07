#include<stdlib.h>
#include<mqueue.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<time.h>

int main(int argc, char const *argv[])
{
    const char * mq_name = "/my_queue";
    struct mq_attr attr;
    struct timespec timeout;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    clock_gettime(0,&timeout);
    timeout.tv_sec += 5;

    mqd_t mqdes = mq_open(mq_name,O_CREAT | O_RDWR | O_EXCL,0644,&attr);
    if(mqdes == (mqd_t)-1){
        perror("mq_open");
        exit(1);
    }

    char send_buf[256];
    while(mqdes != (mqd_t)-1){
        memset(send_buf,0,sizeof(send_buf));
        ssize_t count = read(STDIN_FILENO,send_buf,sizeof(send_buf));
        if(count == -1){
            perror("read");
            exit(1);
        }else if(count == 0){//模拟控制台输入CTRL+D
            printf("读取到EOF，正在退出……\n");
            char eof = EOF;
            mq_timedsend(mqdes,&eof,1,0,&timeout);
            break;
        }
        if(mq_timedsend(mqdes,send_buf,strlen(send_buf),0,&timeout) == -1){
            perror("mq_timedsend");
            exit(1);
        }
        printf("正在发送给接收方……\n");
    }
    
    mq_close(mqdes);

    mq_unlink(mq_name);

    return 0;
}
