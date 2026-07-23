#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
int num =0;
int main(int argc, char const *argv[])
{
    __pid_t pid = fork();
    if(pid == 0){
        num =1;
        printf("Child process num is %d\n",num);
    }else{
        sleep(1);
        printf("Parent process num is %d\n",num);
    }
    return 0;
}
