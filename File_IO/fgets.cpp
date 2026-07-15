#include <stdio.h>

int main(int argc, char const *argv[])
{
    
    FILE * file = fopen("HelloWorld.txt","r");
    char buf[2];
    fgets(buf,sizeof(buf),file);
    
    printf("\n");

    fclose(file);

    return 0;
}
