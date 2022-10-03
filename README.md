## File List
main.c - main src file
main - main executable
sample 1 - 4.txt - sample programs 
output.txt - txt file that shows correct output for sample 1 - 4
Summary - .md and .pdf summary files describe the project in more detail

## How to Compile and Run
In the terminal, type 'gcc main.c -o main'. This compiles the program.
To run the program, type './main samplex.txt y'. Replace x with the sample number and y with your desired interrupt timer. y must be greater than 1. Every y instructions the CPU will push the current registers to the stack and execute the timer interrupt handler. 

*NOTE: When testing sample 3, if you choose a interrupt timer that is a power of 2 (i.e. 2, 4, 8, 16, 32....) the cpu gets
stuck in an infinite loop and you'll have to use ctrl+c to quit the program. Details are in the summary.