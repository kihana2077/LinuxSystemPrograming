#include<stdlib.h>
#include<mqueue.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<time.h>

int main(void)
{
    const char * mq_name = "/my_queue";
    struct mq_attr attr;
    struct timespec timeout;
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    clock_gettime(0,&timeout);
    timeout.tv_sec += 500;

    mqd_t mqdes = mq_open(mq_name,O_RDONLY,0644,&attr);

    char receive_buf[256];
    while(1){
        ssize_t count = mq_timedreceive(mqdes,receive_buf,sizeof(receive_buf),0,&timeout);
        if(count == (ssize_t)-1){
            perror("mq_timedreceive");
            exit(1);
        }
        if(receive_buf[0] == EOF){
            printf("接收完成，即将退出……\n");
            exit(1);
        }
        printf("message:%s\n",receive_buf);

    }
    return 0;
}
