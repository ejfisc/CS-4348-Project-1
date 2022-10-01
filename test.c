#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <string.h>
#include <time.h>

int parseValue(char *line);
int cpuRead(int address);

int p1[2]; // memory -> CPU pipe
int p2[2]; // CPU -> memory pipe 

int main(int argc, char *argv[]) {
    // there should be 2 arguments, main.exe and program file name
    if(argc != 3) {
        printf("3 arguments required");
        return 1000;
    }

    srand(time(NULL));

    if(pipe(p1) == -1) { return 1001; } // open pipe and return code if error
    if(pipe(p2) == -1) { return 1001; }

    int pid = fork();
    if(pid == -1) { return 1002; } // fork process and return if error

    if(pid == 0) {

        int memory[2000]; //0-999 for user program, 1000-1999 for system code

        // read user program into memory
        FILE *program = fopen(argv[1], "r");
        if(program == NULL) { return 1003; }
        printf("Program: \n");
        char line[100];
        int address = 0;
        // read file line by line
        while(fgets(line, 100, program)) { 
            if(line[0] == '\n') {
                continue; // move to next line if line is empty
            }
            else if(line[0] == '.') {
                // update address if first char is a .
                address = parseValue(line); 
            }
            else {
                // store opcode/value into memory array
                int value = parseValue(line);
                memory[address] = value;
                address++;
            }
            
        }
        fclose(program); // done reading, close file
        
        close(p1[0]); // memory doesn't need to read from p1
        close(p2[1]); // memory doesn't need to write to p2
        
        // instruction fetch
        while(read(p2[0], &address, sizeof(address)) != - 1) {
            //printf("Received %u\n", address);
            int val = memory[address];
            if(write(p1[1], &val, sizeof(val)) == -1) { return 1004; } // write to pipe1 and return code if error
            //printf("Wrote %u\n", val);
        }
            
        close(p1[1]);
        close(p2[0]);
    } else {
        int IR = 0;
        int address = 0;

        int instructionCount = 1;
        char *ch = argv[2];
        int interruptTimer = *ch - '0';
        //instruction fetch loop
        while(address >= 0 && address <= 1999) {
            if(instructionCount%interruptTimer == 0) {
              // handle timer interrupt
              printf("timer interrupt handler\n");
            }
            // fetch next instruction
            IR = cpuRead(address);
            address++;
            instructionCount++;
            
            switch (IR) {
              case 1: 
                printf("1\n");
                break;
              case 0:
                printf("0\n");
                break;
              case 14:
                printf("14\n");
                break;
              case 4:
                printf("4\n");
                break;
              case 32:
                printf("32\n");
                break;
              case 21:
                printf("21\n");
                break;
              default:
                return 0;
            }
            
        }
    }


}

// returns the opcode or operand in the line as an integer value
int parseValue(char *line) {
    char* restOfLine;
    // strtol wouldn't work if the first char isn't a digit so we must check if the line is updating the address 
    // (i.e. first character is a period) and if so copy the line over to a new line without that first character
    if(!isdigit(line[0])) {
        int size = strlen(line);
        char newLine[size - 1];
        for(int i = 1; i <= size - 1; i++) {
            newLine[i-1] = line[i]; 
        }
        long longV = strtol(newLine, &restOfLine, 10); // returns a base 10 number found in newLine as a long integer
        return (int)(longV);
    }
    else {
        long longV = strtol(line, &restOfLine, 10); // returns a base 10 number found in line as a long integer
        return (int)(longV);
    }
}

// returns the next opcode or operand at the given address
int cpuRead(int address) {
    close(p1[1]); // CPU doesn't need to write to p1
    close(p2[0]); // CPU doesn't need to read from p2

    int result;
    if(write(p2[1], &address, sizeof(address)) == -1) { return 1004; } // write to pipe2 and return code if error
    //printf("Wrote %u\n", address);
    if(read(p1[0], &result, sizeof(result)) == -1) { return 1005; } // read from pipe1 and return code if error
    //printf("Result is %u\n", result);
    return result;
}