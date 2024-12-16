#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h> 

int x = 11;
int main(){
    int pid;
    pid = fork();
    if(pid == 0){ /* child process */
        x += 15;
        return 0;
    } else if (pid > 0){ /* parent process */
        wait(NULL);
        printf("PARENT: value = %d", x); /* What is the output of this line? */
        return 0;
    }
}