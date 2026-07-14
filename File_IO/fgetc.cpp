#include <stdio.h>

int main(int argc, char const *argv[])
{
    
    FILE * file = fopen("HelloWorld.txt","r");

    //读取文件内容
    int get = fgetc(file);
    //返回一个字节，以int形式
    //如果出错或者读到文件末尾则返回EOF
    printf("读到了字符  %c  \n",get);

    while(get != EOF){
        printf("%c",get);
        get = fgetc(file);
    }
    printf("\n");

    fclose(file);

    return 0;
}
