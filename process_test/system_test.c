#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    int result = system("ping -c 5 baidu.com");
    if(result > 0){
        perror("system");
        exit(EXIT_FAILURE);
    }
    return 0;
}

