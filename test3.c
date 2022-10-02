#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main() {
    char *line = "  ";
    int lineIndex = 0;
    int numberIndex = 0;
    char number[10];
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
    printf("%d", value);
    return value;
}
