#include <stdio.h>
#include <string>
int main(int argc, char const *argv[])
{
    //str 字符串 文件名
    //mode 访问模式
    //return FILE* 结构体指针
    const char * filename = "HelloWorld.txt";
    FILE * file = fopen(filename,"r");
    if(file == NULL){
        printf("无对应文件！！\n");
    }else{
        printf("打开成功！！\n");
    }
    return 0;
}

