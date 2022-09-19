#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

int main(int argc, char *argv[]) {
    // there should be 2 arguments, main.exe and program file name
    if(argc != 2) {
        printf("usage: %s filename");
        return 0;
    }

    int p1[2]; // memory -> CPU pipe
    int p2[2]; // CPU -> memory pipe

    if(pipe(p1) == -1) { return 1001; } // open pipe and return code if error
    if(pipe(p2) == -1) { return 1002; } 

    int pid = fork();
    if(pid == -1) { return 1003; } // fork process and return if error

    if (pid == 0) {
        // memory code

        // memory array:
        int memory[2000]; //0-999 for user program, 1000-1999 for system code
        memory[500]  = 45; // place 45 in address 500 for testing

        // read user program into memory
        FILE *program = fopen(argv[1], "r");
        if(program == NULL) { return 1008; }
        

        close(p1[0]); // memory doesn't need to read from p1
        close(p2[1]); // memory doesn't need to write to p2
        
        // instruction fetch
        int address;
        if(read(p2[0], &address, sizeof(address)) == - 1) { return 1004; } // read from pipe2 and return code if error
        printf("Received %u\n", address);
        int val = memory[address];
        if(write(p1[1], &val, sizeof(val)) == -1) { return 1005; } // write to pipe1 and return code if error
        printf("Wrote %u\n", val);    
        close(p1[1]);
        close(p2[0]);

    } else {
        // CPU code

        // registers:
        int PC; // program counter
        int SP; // stack pointer
        int IR; // instruction register
        int AC; // accumulator
        int X; // user register
        int Y; // user register
        
        close(p1[1]); // CPU doesn't need to write to p1
        close(p2[0]); // CPU doesn't need to read from p2

        // instruction fetch
        int address = 500;
        if(write(p2[1], &address, sizeof(address)) == -1) { return 1006; } // write to pipe2 and return code if error
        printf("Wrote %u\n", address);
        int result;
        if(read(p1[0], &result, sizeof(result)) == -1) { return 1007; } // read from pipe1 and return code if error
        printf("Result is %u\n", result);
        close(p1[0]);
        close(p2[1]);
        wait(NULL);
        
   }    
}


