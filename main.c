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
    if(argc != 2) {
        printf("2 arguments required");
        return 1000;
    }
    
    srand(time(NULL));

    if(pipe(p1) == -1) { return 1001; } // open pipe and return code if error
    if(pipe(p2) == -1) { return 1002; }

    int pid = fork();
    if(pid == -1) { return 1003; } // fork process and return if error

    if (pid == 0) {
        // memory code
        int memory[2000]; //0-999 for user program, 1000-1999 for system code

        // read user program into memory
        FILE *program = fopen(argv[1], "r");
        if(program == NULL) { return 1008; }
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
            printf("Received %u\n", address);
            int val = memory[address];
            if(write(p1[1], &val, sizeof(val)) == -1) { return 1005; } // write to pipe1 and return code if error
            printf("Wrote %u\n", val);
        }
            
        close(p1[1]);
        close(p2[0]);

    } else {
        // CPU code

        // registers
        int PC; // program counter
        int SP; // stack pointer
        int IR; // instruction register
        int AC; // accumulator
        int X; // user register
        int Y; // user register
        
        
        close(p1[1]); // CPU doesn't need to write to p1
        close(p2[0]); // CPU doesn't need to read from p2

        // address variables
        int address = 0;
        int tempAddr;
        int tempAddr2;
        
        // instruction fetch loop
        while(address >= 0 && address <= 1999) {
            // fetch next instruction
            IR = cpuRead(address);
            address++;
            switch (IR) {
                case 1: // load the value into the AC
                    AC = cpuRead(address);
                    address++;
                    break;
                case 2: // load the value at the address into the AC
                    tempAddr = cpuRead(address); // read the address from the next address
                    AC = cpuRead(tempAddr);
                    address++;
                    break;
                case 3: // load the value from the address found in the given addres into the AC
                    tempAddr = cpuRead(address);
                    tempAddr2 = cpuRead(tempAddr);
                    AC = cpuRead(tempAddr2);
                    address++;
                    break;
                case 4: // load the value at address + X into the AC
                    tempAddr = cpuRead(address);
                    AC = cpuRead(tempAddr + X);
                    address++;
                    break;
                case 5: // load the value at address + Y into the AC
                    tempAddr = cpuRead(address);
                    AC = cpuRead(tempAddr + Y);
                    address++;
                    break;
                case 6: // Load from (SP+X) into the AC
                    AC = cpuRead(SP + X);
                    break;
                case 7: // Store the value in the AC into the address 
                    // need to figure out cpu write
                    break;
                case 8: // gets a random into from 1 to 100 into the AC
                    AC = rand() % 101;
                    break;
                case 9: // if port=1, writes AC as an int to the screen, if port=2, writes AC as char to the screen
                    // need to figure out what this means
                    break;
                case 10: // add the value in X to the AC
                    AC += X;
                    break;
                case 11: // add the value in Y to the AC
                    AC += Y;
                    break;
                case 12: // subtract the value in X from the AC
                    AC -= X;
                    break;
                case 13: // subtract the value in Y from the AC
                    AC -= Y;
                    break;
                case 14: // Copy the value in the AC to X
                    X = AC;
                    break;
                case 15: // Copy the value in X to the AC
                    AC = X;
                    break;
                case 16: // Copy the value in the AC to Y
                    Y = AC;
                    break;
                case 17: // copy the value in Y to the AC
                    AC = Y;
                    break;
                case 18: // copy the value in AC to the SP
                    SP = AC;
                    break;
                case 19: // Copy the value in SP to the AC
                    AC = SP;
                    break;
                case 20: // jump to the address
                    tempAddr = cpuRead(address);
                    address = tempAddr;
                    break;
                case 21: // jump to the address only if the value in the AC is zero
                    if(AC == 0) {
                        tempAddr = cpuRead(address);
                        address = tempAddr;
                        break;
                    }
                    else {
                        address += 2; // increment by 2 so it skips the address/operand of this instruction
                        break;
                    }
                case 22: // jump to the address only if the value in the AC is not zero
                    if(AC != 0) {
                        tempAddr = cpuRead(address);
                        address = tempAddr;
                        break;
                    }
                    else {
                        address += 2; // increment by 2 so it skips the address/operand of this instruction
                        break;
                    }
                case 23: // push return address onto stack, jump to the address
                    // need to figure out write and stack
                    break;
                case 24: // pop return address from the stack, jump to the address
                    // need to figure out stack stuff
                    break;
                case 25: // increment the value in X
                    X++;
                    break;
                case 26: // decrement the value in X
                    X--;
                    break;
                case 27: // push AC onto stack
                    // need to figure out stack
                    break;
                case 28: // pop from stack onto AC
                    // need to figure out stack
                    break;
                case 29: // perform system call
                    // need to figure out what this means
                    break;
                case 30: // return from system call
                    // need to figure out what this means
                    break;
                case 50: // end execution
                    printf("End of simulation");
                    _exit(0);
                    break;
                default: return 0;
            }
        }

        


        close(p1[0]);
        close(p2[1]);
        wait(NULL);
        
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
    if(write(p2[1], &address, sizeof(address)) == -1) { return 1006; } // write to pipe2 and return code if error
    printf("Wrote %u\n", address);
    if(read(p1[0], &result, sizeof(result)) == -1) { return 1007; } // read from pipe1 and return code if error
    printf("Result is %u\n", result);
    return result;
}