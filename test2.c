#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <string.h>
#include <time.h>

struct cpuRequest {
    char op;
    int address;
};

int main() {
    int p1[2];
    int p2[2];

    pipe(p1);
    pipe(p2);

    int pid = fork();
    if(pid == 0) {
        close(p1[0]); // memory doesn't need to read from p1
        close(p2[1]); // memory doesn't need to write to p2
        struct cpuRequest req;
        while(read(p2[0], &req, sizeof(req)) != 0) {
            switch(req.op) {
                case 'r':
                    printf("read from %d\n", req.address);
                    break;
                case 'w':
                    printf("write to %d\n", req.address);
                    break;
                default: 
                    printf("default");
            }
        }
    } else {
        close(p1[1]); // CPU doesn't need to write to p1
        close(p2[0]); // CPU doesn't need to read from p2
        struct cpuRequest req;
        for(int i = 0; i < 10; i++) {
            if(i%2 == 0) {
                req.op = 'r';
            }
            else {
                req.op = 'w';
            }
            req.address = i;
            write(p2[1], &req, sizeof(req));
        }

    }
}