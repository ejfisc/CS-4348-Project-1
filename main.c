#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

int pfds[2]; // pipe descriptor

int main() {    
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
        // memory code
        int memory[2000]; //0-999 for user program, 1000-1999 for system code
        memory[500]  = 45; // place 45 in address 500 for testing

        // split into a second process for reading
        result = fork();
        if(result == -1)
            exit(1);

        if(result == 0) {
            // read address from pipe and then write value at that address to pipe
            int address;
            read(pfds[0], &address, sizeof(address));
            printf("memory read %u from pipe\n", address);
            int value = memory[address];
            printf("memory writing %u to pipe\n", value);
            write(pfds[1], &value, sizeof(value));
            _exit(0);
        }
        else {
            waitpid(-1, NULL, 0); // wait for child process to finish read/write
        }
        

        /* read address from pipe, then read value from pipe, then write the value to the address in memory
        //waitpid(-1, NULL, 0); // wait for cpu to write address to pipe
        read(pfds[0], &address, sizeof(address));
        printf("memory read address: %u from pipe\n", address);
        //waitpid(-1, NULL, 0); // wait for cpu to write value to pipe
        read(pfds[0], &value, sizeof(value));
        printf("memory read value: %u from pipe\n", value);
        memory[address] = value;
        printf("memory[%u]: %u", address, value);
        _exit(0);*/
    }
    else
    {
        // cpu code
        // registers:
        int PC; // program counter
        int SP; // stack pointer
        int IR; // instruction register
        int AC; // accumulator
        int X; // user register
        int Y; // user register

        printf("testing memory read\n");
        // write address to pipe and then read value of that address from pipe
        int address = 500;
        printf("CPU writing %u to pipe\n", address);
        write(pfds[1], &address, sizeof(address));
        //waitpid(-1, NULL, 0); // wait for memory to read address and write value to pipe
        int value;
        read(pfds[0], &value, sizeof(value));
        printf("CPU read %u from pipe\n", value);
        waitpid(-1, NULL, 0);

        /*
        printf("testing memory write\n");
        // send value and address to pipe to write to memory
        address = 500;
        value = 60;
        printf("CPU writing address: %u to pipe\n", address);
        write(pfds[1], &address, sizeof(address));
        //waitpid(-1, NULL, 0); // wait for memory to read address
        printf("CPU writing value: %u to pipe\n", value);
        write(pfds[1], &value, sizeof(value));
        //waitpid(-1, NULL, 0); // wait for memory to read value*/
        
   }    
}


