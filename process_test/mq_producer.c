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
    attr.mq_curmsgs = 0;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;

    mqd_t mqdes = mq_open(mq_name,O_CREAT | O_RDWR | O_EXCL,0644,&attr);

    

    return 0;
}
