#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

int main() {
    int pfds[2];
    int result;

    result = pipe(pfds);
    // check if pipe failed
    if (result == -1)
        exit(1);

    result = fork();
    // check if fork failed
    if (result == -1)
        exit(1);

    if (result == 0) 
    {
        printf("memory: writing to the pipe\n");
        int address = 42;
        write(pfds[1], &address, sizeof(address));
        printf("memory: exiting\n");
        _exit(0);
    }
    else
    {
        waitpid(-1, NULL, 0);
        printf("CPU: reading from pipe\n");
        int address;
        read(pfds[0], &address, sizeof(address));
        printf("CPU: read \"%u\"\n", address);
        
   }
}