#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
int main(int argc, char const *argv[])
{
    int fd = open("HelloWorld.txt",O_RDONLY | O_CREAT,0664);
    //char * path 打开文件的路径
    //int oflag :打开文件的模式
    /*  O_RDONLY 只读模式
        O_WRONLY 只写模式
        O_RDWR
        O_CREATE
        O_APPEND
        O_TRUNC
        ... 可变参数(创建文件的权限类似chmod)
        return int 文件描述符 打开文件失败则返回-1
    
    */
   printf("%d",fd);
    return 0;
}
