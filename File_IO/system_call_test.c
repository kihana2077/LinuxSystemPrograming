#include<unistd.h>
#include<fcntl.h>
#include <stdio.h>
#include<stdlib.h>

int main(int argc, char const *argv[])
{
    int fd = open("HelloWorld.txt",O_RDONLY);
    if(fd == -1){
        printf("打开文件失败了\n");
        //_exit(1);//是系统调用，不提供任何清理操作
        exit(EXIT_FAILURE);//这是stdlib库函数
    }

    return 0;
}
