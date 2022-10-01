#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <string.h>
#include <time.h>

/*Error Codes:
1000 - argument requirement not fulfilled
1001 - pipe failed to open
1002 - process failed to fork
1003 - program file is null
1004 - write to pipe failed
1005 - read from pipe failed
1006 - invalid cpu request
1007 - memory violation
*/

//cpuRequest struct, cpu requests to either read from an address in memory or write a value to an address in memory
struct cpuRequest {
    char op; //'r' = read, 'w' = write
    int address;
    int value;
};

// initialize helper methods, definitions below main()
int parseValue(char *line);
int cpuRead(int address);
int cpuWrite(int address, int value);

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

    if (pid == 0) {
        // memory code
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
        
        struct cpuRequest req;
        int val;

        // instruction fetch loop
        while(read(p2[0], &req, sizeof(req)) != - 1) {
            switch (req.op) {
                case 'r':
                    val = memory[req.address];
                    if(write(p1[1], &val, sizeof(val)) == -1) { return 1004; } // write to pipe1 and return code if error
                    break;
                case 'w':
                    memory[req.address] = req.value;
                    break;
                default:
                    printf("invalid cpu request");
                    return 1006;
            }
        }
            
        close(p1[1]);
        close(p2[0]);

    } else {
        // CPU code

        // registers
        int PC = 0; // program counter
        int SP = 999; // stack pointer
        int IR; // instruction register
        int AC; // accumulator
        int X; // user register
        int Y; // user register
        
        
        close(p1[1]); // CPU doesn't need to write to p1
        close(p2[0]); // CPU doesn't need to read from p2

        // address variables
        int tempAddr;
        int tempAddr2;

        int instructionCount = 1;
        char *ch = argv[2];
        int interruptTimer = *ch - '0'; // subtracting the ascii value of 0 turns the character argument into an int
        
        int port; // used for putting the AC as a char or int to screen

        bool kernelMode = false;
        
        // instruction fetch loop
        while(PC >= 0 && PC <= 1999) {
            // timer interrupt every x instructions if interrupts are allowed (i.e., if we're already executing the
            // interrupt handler instructions, don't interrupt the interrupt handler)
            if(instructionCount%interruptTimer == 0 && !kernelMode) {
                // handle timer interrupt
                int userStack = SP;
                SP = 1999; // set stack pointer to start of system stack
                cpuWrite(SP, userStack); // save user stack pointer to system stack
                SP--; // decrement stack pointer (stack grows down)
                cpuWrite(SP, PC); // save stack pointer to stack
                PC = 1000; // set program counter to timer interrupt handler address
                /* current system stack (SP = 1998)
                    1999 - SP
                    1998 - PC
                */
                kernelMode = true;
            }
            // fetch next instruction
            IR = cpuRead(PC);
            PC++;
            instructionCount++;
            switch (IR) {
                case 1: // load the value into the AC
                    AC = cpuRead(PC);
                    PC++;
                    break;
                case 2: // load the value at the address into the AC
                    tempAddr = cpuRead(PC); // read the address from the next address
                    AC = cpuRead(tempAddr);
                    PC++;
                    break;
                case 3: // load the value from the address found in the given addres into the AC
                    tempAddr = cpuRead(PC);
                    tempAddr2 = cpuRead(tempAddr);
                    AC = cpuRead(tempAddr2);
                    PC++;
                    break;
                case 4: // load the value at address + X into the AC
                    tempAddr = cpuRead(PC);
                    AC = cpuRead(tempAddr + X);
                    PC++;
                    break;
                case 5: // load the value at address + Y into the AC
                    tempAddr = cpuRead(PC);
                    AC = cpuRead(tempAddr + Y);
                    PC++;
                    break;
                case 6: // Load from (SP+X) into the AC
                    AC = cpuRead(SP + X);
                    break;
                case 7: // Store the value in the AC into the address 
                    tempAddr = cpuRead(PC);
                    if(tempAddr >= 1000 && !kernelMode) {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    cpuWrite(tempAddr, AC);
                    PC++;
                    // need to figure out cpu write
                    break;
                case 8: // gets a random into from 1 to 100 into the AC
                    AC = rand() % 101;
                    break;
                case 9: // if port=1, writes AC as an int to the screen, if port=2, writes AC as char to the screen
                    port = cpuRead(PC);
                    if(port == 1) 
                        printf("%d", AC);
                    else
                        printf("%c", AC);
                    PC++;
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
                    PC = cpuRead(PC);
                    if(PC >= 1000 && !kernelMode) {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    break;
                case 21: // jump to the address only if the value in the AC is zero
                    if(AC == 0) 
                        PC = cpuRead(PC);
                        if(PC >= 1000 && !kernelMode) {
                            printf("Memory Violation: User program cannot access system memory.");
                            return 1007;
                        }
                    else
                        PC++; // increment program counter to skip operand to next instruction
                    break;
                case 22: // jump to the address only if the value in the AC is not zero
                    if(AC != 0) 
                        PC = cpuRead(PC);
                        if(PC >= 1000 && !kernelMode) {
                            printf("Memory Violation: User program cannot access system memory.");
                            return 1007;
                        }
                    else 
                        PC++; // increment program counter to skip operand to next instruction
                    break;
                case 23: // push return address onto stack, jump to the address
                    cpuWrite(SP, PC); // push return address onto stack
                    SP--;
                    PC++;
                    PC = cpuRead(PC); // set program counter to address
                    break;
                case 24: // pop return address from the stack, jump to the address
                    PC = cpuRead(SP);
                    SP++;
                    break;
                case 25: // increment the value in X
                    X++;
                    break;
                case 26: // decrement the value in X
                    X--;
                    break;
                case 27: // push AC onto stack
                    SP--;
                    cpuWrite(SP, AC);
                    break;
                case 28: // pop from stack onto AC
                    AC = cpuRead(SP);
                    SP++;
                    break;
                case 29: // perform system call
                    // don't perform system call if in kernel mode
                    if(!kernelMode) {
                        int userStack = SP;
                        SP = 1999; // set stack pointer to start of system stack
                        cpuWrite(SP, userStack); // save user stack pointer to system stack
                        SP--; // decrement stack pointer (stack grows down)
                        cpuWrite(SP, PC); // save stack pointer to stack
                        PC = 1500; // set program counter to system call handler address
                        /* current system stack (SP = 1998)
                            1999 - SP
                            1998 - PC
                        */
                        kernelMode = true;
                    }
                    break;
                case 30: // return from system call
                    PC = cpuRead(SP); // PC is on top of stack
                    SP++;
                    SP = cpuRead(SP); // SP is on bottom of stack
                    kernelMode = false;
                    break;
                case 50: // end execution
                    printf("End of simulation");
                    _exit(0);
                    return 0;
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

    struct cpuRequest req;
    req.op = 'r';
    req.address = address;
    int result;
    if(write(p2[1], &req, sizeof(req)) == -1) { return 1004; } // write to pipe2 and return code if error
    if(read(p1[0], &result, sizeof(result)) == -1) { return 1005; } // read from pipe1 and return code if error
    return result;
}

// writes the given value to the given address in memory
int cpuWrite(int address, int value) {
    close(p1[1]); // CPU doesn't need to write to p1
    close(p2[0]); // CPU doesn't need to read from p2

    struct cpuRequest req;
    req.op = 'w';
    req.address = address;
    req.value = value;
    if(write(p2[1], &req, sizeof(req)) == -1) { return 1004; } // write to pipe2 and return code if error
    return 0;
}