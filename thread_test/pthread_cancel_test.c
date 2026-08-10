#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>

void *test(void *args){
    pthread_setcancelstate(PTHREAD_CANCEL_DEFERRED,NULL);
    //设置取消方式是每个线程自己的属性，
        printf("working......\n");
        printf("1秒钟后即将取消该线程......\n");
        sleep(3);
    pthread_testcancel();
    printf("通过测试的取消点函数取消\n");
    return NULL;
}
int main(int argc, char const *argv[])
{
    
    pthread_t pid;
    pthread_create(&pid,NULL,test,NULL);
    printf("Main running......\n");
    sleep(1);
    pthread_cancel(pid);
    pthread_join(pid,NULL);
    //取消之后还要使用join回收资源
    return 0;
}
