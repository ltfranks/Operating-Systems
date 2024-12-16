#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void){
    
    int i =0;

    while (fork() && i < 2){
        wait(NULL);
        printf("Hi");
        fflush(stdout);
        i++;
    }

    printf("%d", i);
    return 0;
}