/* Ethan Fischer CS 4348.005 Project 1 Fall 2022 9/14 */
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
    char op; //'r' = read, 'w' = write, 'n' = no op
    int address;
    int value;
    int debugPID;
};

// initialize helper methods, definitions below main()
int parseValue(char *line);
int cpuRead(int address);
int cpuWrite(int address, int value);
void push(int *SP, int value);
int pop(int *SP);

int p1[2]; // memory -> CPU pipe
int p2[2]; // CPU -> memory pipe 


int debugPID = 0;
int debugOutput = 0;


int main(int argc, char *argv[]) {
    // there should be 2 arguments, main.exe and program file name
    if(argc != 3) {
        printf("3 arguments required");
        return 1000;
    }
    char file[32] = "\0";
    strcpy(file, argv[1]);
    char *ch = argv[2];
    int interruptTimer = atoi(ch);

    srand(time(NULL));

    if(pipe(p1) == -1) { return 1001; } // open pipe and return code if error
    if(pipe(p2) == -1) { return 1001; }

    int pid = fork();
    if(pid == -1) { return 1002; } // fork process and return if error

    if (pid == 0) {
        // memory code

        int memory[2000]; //0-999 for user program, 1000-1999 for system code
        memset(memory, 0, sizeof(memory)); // wipe memory


        // read user program into memory
        FILE *program = fopen(file, "r");
        if(program == NULL) { return 1003; }
        char line[50] = {'\0'};
        int address = 0;
        // read file line by line
        while(fgets(line, 50, program)) {
            if(line[0] == '.') {
                address = parseValue(line);
            }
            else if(isspace(line[0])) {
                continue;
            }
            else {
                memory[address] = parseValue(line);
                address++;
            }
        }
        fclose(program); // done reading, close file

        close(p1[0]); // memory doesn't need to read from p1
        close(p2[1]); // memory doesn't need to write to p2
        
        int breakoutCounter = 0;

        struct cpuRequest req;
        req.op = 'n';
        req.address = 0;
        req.value = 0;

        // instruction fetch loop
        while(read(p2[0], &req, sizeof(req)) != - 1 && breakoutCounter < 2) {
            switch (req.op) {
                case 'r':
                {
                    int val = memory[req.address];
                     if(debugOutput) printf("[%d] read value: %d from address: %d\n", req.debugPID, val, req.address);
                    if(write(p1[1], &val, sizeof(val)) == -1) { return 1004; } // write to pipe1 and return code if error
                    break;
                }
                case 'w':
                {
                    memory[req.address] = req.value;
                     if(debugOutput) printf("[%d] write value: %d at address: %d\n", req.debugPID, req.value, req.address);
                    break;
                }
                case 'n':
                {
                    breakoutCounter++;
                    break; //do nothing
                }
                default:
                {
                    // printf("invalid cpu request");
                    // return 1006;
                }
            }
        }
            
        // memory broke out of read loop because it didn't get any cpu requests
        if(breakoutCounter >= 2)
            printf("nothing to do for too long");
        
        // close pipes
        close(p1[1]);
        close(p2[0]);

    } else {    
        // CPU code
        
        // registers
        int PC = -1; // program counter (set to -1 because it's incremented at the beginning of the fetch loop, this way the first fetch comes from 0 and not 1)
        int SP = 999; // stack pointer
        int IR = 0; // instruction register
        int AC = 0; // accumulator
        int X = 0; // user register
        int Y = 0; // user register
        
        close(p1[1]); // CPU doesn't need to write to p1
        close(p2[0]); // CPU doesn't need to read from p2

        // temp variables and counters
        int tempAddr = 0;
        int tempAddr2 = 0;
        int instructionCount = 1;
        int port = 0; 
        bool kernelMode = false;

        // instruction fetch loop
        while (true) {
            // timer interrupt every x instructions if interrupts are allowed (i.e., if we're already executing the
            // interrupt handler instructions, don't interrupt the interrupt handler)
            if (instructionCount%interruptTimer == 0 && !kernelMode) {
                // handle timer interrupt
                int userStack = SP;
                SP = 2000; // SP will get decremented down to correct 1999 in push()
                push(&SP, userStack); // save user stack pointer to system stack                 // decrement stack pointer (stack grows down)
                push(&SP, PC); // save program counter to system stack
                /* current system stack (SP = 1998)
                    1999 - SP
                    1998 - PC
                */
                PC = 999;   // set program counter to timer interrupt handler address
                kernelMode = true;
                if(debugOutput) printf("[%d] ti: PC: %d, SP: %d\n", debugPID, PC, SP);
            }
            
            // fetch next instruction
            PC++;
             if(debugOutput) printf("[%d] PC: %d, ", debugPID, PC);
            IR = cpuRead(PC);
             if(debugOutput) printf("IR: %d, IC: %d\n", IR, instructionCount);
            instructionCount++;
            switch (IR) {
                case 1: // load the value into the AC
                    PC++;
                    AC = cpuRead(PC);
                     if(debugOutput) printf("[%d] case 1: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 2:  // load the value at the address into the AC
                    PC++;
                    tempAddr = cpuRead(PC); // read the address from the next address
                    if (tempAddr >= 1000 && !kernelMode)
                    {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    AC = cpuRead(tempAddr);
                    if(debugOutput) printf("[%d] case 2: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 3: // load the value from the address found in the given addres into the AC
                    PC++;
                    tempAddr = cpuRead(PC);
                    if (tempAddr >= 1000 && !kernelMode)
                    {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    tempAddr2 = cpuRead(tempAddr);
                    if (tempAddr2 >= 1000 && !kernelMode)
                    {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    AC = cpuRead(tempAddr2);
                    if(debugOutput) printf("[%d] case 3: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 4: // load the value at address + X into the AC
                    PC++;
                    tempAddr = cpuRead(PC);
                    AC = cpuRead(tempAddr + X);
                     if(debugOutput) printf("[%d] case 4: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 5: // load the value at address + Y into the AC
                    PC++;
                    tempAddr = cpuRead(PC);
                    AC = cpuRead(tempAddr + Y);
                     if(debugOutput) printf("[%d] case 5: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 6: // Load from (SP+X) into the AC
                    AC = cpuRead(SP + X);
                     if(debugOutput) printf("[%d] case 6: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 7: // Store the value in the AC into the address
                    PC++;
                    tempAddr = cpuRead(PC);
                    if (tempAddr >= 1000 && !kernelMode)
                    {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    cpuWrite(tempAddr, AC);
                     if(debugOutput) printf("[%d] case 7: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 8: // gets a random into from 1 to 100 into the AC
                    AC = rand() % 101;
                     if(debugOutput) printf("[%d] case 8: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 9: // if port=1, writes AC as an int to the screen, if port=2, writes AC as char to the screen
                    PC++;
                    port = cpuRead(PC);
                    if(port == 1)
                        printf("%d", AC);
                    else
                        printf("%c", AC);
                     if(debugOutput) printf("[%d] case 9: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 10: // add the value in X to the AC
                    AC += X;
                     if(debugOutput) printf("[%d] case 10: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 11: // add the value in Y to the AC
                    AC += Y;
                     if(debugOutput) printf("[%d] case 11: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 12: // subtract the value in X from the AC
                    AC -= X;
                     if(debugOutput) printf("[%d] case 12: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 13: // subtract the value in Y from the AC
                    AC -= Y;
                     if(debugOutput) printf("[%d] case 13: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 14: // Copy the value in the AC to X
                    X = AC;
                     if(debugOutput) printf("[%d] case 14: PC: %d, X: %d\n", debugPID, PC, X);
                    break;
                case 15: // Copy the value in X to the AC
                    AC = X;
                     if(debugOutput) printf("[%d] case 15: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 16: // Copy the value in the AC to Y
                    Y = AC;
                     if(debugOutput) printf("[%d] case 16: PC: %d, Y: %d\n", debugPID, PC, Y);
                    break;
                case 17: // copy the value in Y to the AC
                    AC = Y;
                     if(debugOutput) printf("[%d] case 17: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 18: // copy the value in AC to the SP
                    SP = AC;
                     if(debugOutput) printf("[%d] case 18: PC: %d, SP: %d\n", debugPID, PC, SP);
                    break;
                case 19: // Copy the value in SP to the AC
                    AC = SP;
                     if(debugOutput) printf("[%d] case 19: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 20: // jump to the address
                    PC++;
                    PC = cpuRead(PC);
                    if (PC >= 1000 && !kernelMode)
                    {
                        printf("Memory Violation: User program cannot access system memory.");
                        return 1007;
                    }
                    if(debugOutput) printf("[%d] case 20: PC: %d, AC: %d\n", debugPID, PC, AC);
                    PC--;
                    break;
                case 21: // jump to the address only if the value in the AC is zero
                    if (AC == 0) {
                        PC++;
                        PC = cpuRead(PC);
                        if (PC >= 1000 && !kernelMode)
                        {
                            printf("Memory Violation: User program cannot access system memory.");
                            return 1007;
                        }
                        PC--;
                    } else
                        PC++; // increment program counter to skip operand to next instruction
                     if(debugOutput) printf("[%d] case 21: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 22: // jump to the address only if the value in the AC is not zero
                    if (AC != 0) {
                        PC++;
                        PC = cpuRead(PC);
                        if (PC >= 1000 && !kernelMode)
                        {
                            printf("Memory Violation: User program cannot access system memory.");
                            return 1007;
                        }
                        PC--;
                    } else
                        PC++; // increment program counter to skip operand to next instruction
                     if(debugOutput) printf("[%d] case 22: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 23:
                    // SP--;              // push return address onto stack, jump to the address
                    push(&SP, PC+2); // push return address () onto stack
                    // SP--;
                    PC++;
                    PC = cpuRead(PC); // set program counter to address
                     if(debugOutput) printf("[%d] case 23: PC: %d, SP: %d\n", debugPID, PC, SP);
                    PC--; // PC will increment back up at beginning of loop
                    break;
                case 24: // pop return address from the stack, jump to the address
                    PC = pop(&SP);
                    // SP++;
                    if(debugOutput) printf("[%d] case 24: PC: %d, SP: %d\n", debugPID, PC, SP);
                    PC--;
                    break;
                case 25: // increment the value in X
                    X++;
                     if(debugOutput) printf("[%d] case 25: PC: %d, X: %d\n", debugPID, PC, X);
                    break;
                case 26: // decrement the value in X
                    X--;
                     if(debugOutput) printf("[%d] case 26: PC: %d, X: %d\n", debugPID, PC, X);
                    break;
                case 27: // push AC onto stack
                    push(&SP, AC);
                    // SP--;
                     if(debugOutput) printf("[%d] case 27: PC: %d, SP: %d\n", debugPID, PC, SP);
                    break;
                case 28: // pop from stack onto AC
                    // SP++;
                    AC = pop(&SP);
                     if(debugOutput) printf("[%d] case 28: PC: %d, AC: %d\n", debugPID, PC, AC);
                    break;
                case 29: // perform system call
                    // don't perform system call if in kernel mode
                    if (!kernelMode)
                    {
                        int userStack = SP;
                        SP = 1999;             // set stack pointer to start of system stack
                        push(&SP, userStack); // save user stack pointer to system stack
                        // SP--;                    // decrement stack pointer (stack grows down)
                        push(&SP, PC);        // save stack pointer to stack
                        // SP--;
                        /* current system stack (SP = 1997)
                            1999 - SP
                            1998 - PC
                        */
                        PC = 1499;               // set program counter to system call handler address
                        kernelMode = true;
                        if(debugOutput) printf("[%d] case 29: PC: %d, SP: %d\n", debugPID, PC, SP);
                    }
                     
                    break;
                case 30: // return from system call
                    // SP++;
                    PC = pop(&SP); // PC is on top of stack
                    // SP++;
                    SP = pop(&SP); // SP is on bottom of stack
                    kernelMode = false;
                    if(debugOutput) printf("[%d] case 30: PC: %d, SP: %d\n", debugPID, PC, SP);
                    break;
                case 50: // end execution
                    printf("End of simulation");
                     if(debugOutput) printf("[%d] case 50: PC: %d, AC: %d\n", debugPID, PC, AC);
                    _exit(0);
                    return 0;
                default:
                     if(debugOutput) printf("[%d] case null: PC: %d, IR: %d\n", debugPID, PC, IR);
                    return 0;
            }
        }
        struct cpuRequest stopReq;
        stopReq.op = 'n';
        if(write(p2[1], &stopReq, sizeof(stopReq)) == -1) { return 1004; } // write to pipe2 and return code if error
        close(p1[0]);
        close(p2[1]);
        wait(NULL);
        
    }
    
}    

// returns the opcode or operand in the line as an integer value
int parseValue(char *line) {
    int lineIndex = 0;
    int numberIndex = 0;
    char number[10] = {'\0'};
    //check if first char is a period
    if(line[0] == '.') {
        lineIndex = 1;
    }
    while(isdigit(line[lineIndex])) {
        number[numberIndex] = line[lineIndex];
        lineIndex++;
        numberIndex++;
    }
    int value = atoi(number);
    return value;
}

// returns the next opcode or operand at the given address
int cpuRead(int address) {
    close(p1[1]); // CPU doesn't need to write to p1
    close(p2[0]); // CPU doesn't need to read from p2
    
    struct cpuRequest req;
    req.op = 'r';
    req.address = address;
    req.debugPID = debugPID++;
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
    req.debugPID = debugPID++;
    if(write(p2[1], &req, sizeof(req)) == -1) { return 1004; } // write to pipe2 and return code if error
    return 0;
}

void push(int *SP, int value) {
    (*SP)--;
    cpuWrite(*SP, value);
}

int pop(int *SP) {
    int value = cpuRead(*SP);
    (*SP)++;
    return value;
}