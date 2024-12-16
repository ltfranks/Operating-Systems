#include  <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define ITER_MAX 1000
#define SLEEP 10.0

int main(void){
    char msg_p[] = "parent";
    char msg_c[] = "child";
    char n1[] = "\n";

    for(int i = 0; i<ITER_MAX; i++){
        if(fork()){
            sleep(SLEEP);
            /* printf("parent "); fflush(stdout); 
            write(STDOUT_FILENO, msg_p, sizeof(msg_p)-1); */
            /* printf("%s", msg_p); */
            printf("%s", msg_p);
            fflush(stdout);
            
            wait(NULL);
        } else {
            sleep(SLEEP);
            /* printf("child "); fflush(stdout); 
            write(STDOUT_FILENO, msg_c, sizeof(msg_c)-1); */
            /* when printf("%s", msg_c); is used, we're stuck in parent process */
            printf("%s", msg_c);
            fflush(stdout);
            
            return 0;
        }

        sleep(SLEEP);
        /* printf("%i\n", i); fflush(stdout); */
        write(STDOUT_FILENO, n1, sizeof(n1)-1); 
    }
    return 0;
}