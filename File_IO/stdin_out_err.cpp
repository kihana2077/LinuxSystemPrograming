#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    
    FILE * file = fopen("HelloWorld.txt","r");
    char buf[100];
    fgets(buf,sizeof(buf),stdin);
    
    printf("\n%s",buf);




    fclose(file);

    return 0;
}
