#include <stdio.h>

int main(int argc, char const *argv[])
{
    
    FILE * file = fopen("HelloWorld.txt","r");
    //参数1：文件指针
    //参数2：带有格式化的字符串
    //参数3： ... 可变参数
    //return 成功匹配到的参数的个数  失败则返回0
    char name1[20];
    char name2[20];
    int num;
    int flag = fscanf(file,"%s %d %s",name1,&num,name2);

    printf("匹配到了%d个参数\n",flag);
    printf("%s %d %s\n",name1,num,name2);
    
    printf("\n");

    fclose(file);

    return 0;
}
