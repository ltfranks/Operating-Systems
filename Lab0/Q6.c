#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(void){
pid_t pid1, pid2;
int status;
printf("Before fork 1\n");
pid1 = fork();
printf("After fork 1\n");
if (!pid1)
{
printf("Before fork 2\n");
pid2 = fork();
printf("After fork 2\n");
if (!pid2)
{
printf("Before fork 3\n");
fork();
printf("After fork 3\n");
exit(-1);
}
else
{
wait(&status);
printf("status: %d\n", WEXITSTATUS(status));
}
}
else
{
wait(&status);
printf("status: %d\n", WEXITSTATUS(status));
}
printf("Done!\n");

}