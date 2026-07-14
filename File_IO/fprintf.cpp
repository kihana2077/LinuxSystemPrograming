#include <stdio.h>
#include <string>
int main(int argc, char const *argv[])
{
    //str 字符串 文件名
    //mode 访问模式
    //return FILE* 结构体指针
    const char * filename = "HelloWorld.txt";
    FILE * file = fopen(filename,"a");
    if(file == NULL){
        printf("无对应文件！！\n");
    }else{
        printf("打开成功！！\n");
    }
    
    int flag = fprintf(file,"\nHELLO C++" );
    //文件指针、写入字符串、可变参数
    //return 返回成功写入字符的个数、不包含换行符
    //失败返回EOF
    printf("写入了%d个字节\n",flag);

    int is_CLosed = fclose(file);
    if(is_CLosed == 0){
        printf("关闭文件成功\n");

    }else if(is_CLosed == EOF){
        printf("关闭文件失败\n");
    }
    return 0;
}

