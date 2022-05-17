/**
 Jason Dao (jnd88)
 Donghui Li (dl882)
 vi.cs.rutgers.edu
 CS416
 *
**/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Part 1 - Step 1 and 2: Do your tricks here
 * Your goal must be to change the stack frame of caller (main function)
 * such that you get to the line after "r2 = *( (int *) 0 )"
 */
void segment_fault_handler(int signum) {

    printf("handling segmentation fault!\n");

    /* Step 2: Handle segfault and change the stack*/
    int * signalpointer=&signum;
    signalpointer+=15;
    (*signalpointer)+=5; 
}

int main(int argc, char *argv[]) {

    int r2 = 0;

    /* Step 1: Registering signal handler */
    signal(SIGSEGV, segment_fault_handler);


    r2 = *( (int *) 0 ); // This will generate segmentation fault

    r2 = r2 + 1 * 100;
    printf("result after handling seg fault %d!\n", r2);

    return 0;
}
