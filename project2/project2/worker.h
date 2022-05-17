// File:	worker_t.h

// List all group member's name: Donghui Li, Jason Dao
// username of iLab: jnd88@vi.cs.rutgers.edu
// iLab Server: vi.cs.rutgers.edu

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1
#define QUANTUM 10   //each thread gets QUANTUM time slice in RR(mlfq uses QUANTUM2 as well)
#define QUANTUM2 5 //difference between each time slice in mlfq (level 1 has QUANTUM, 2 has QUANTUM+5, 3 had QUANTUM+10, etc)
#define S 100    //every S milliseconds, increase priority
#define LEVELS 4
#define SCHEDULED 0
#define READY 1
#define BLOCKED 2
#define TERMINATED 3


/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>
#include <signal.h>	
#include <time.h>
#include <string.h>

typedef uint worker_t;

typedef struct TCB {
	/* add important states in a thread control block */
	worker_t threadID;  
	int status; //0:running, 1:ready, 2:block, 3 finished
	ucontext_t context;
	stack_t* stack;
	int priority;
} TCB; 

typedef struct threadNode {
	worker_t threadID; 
	int status;
	struct TCB* TCB;
	int start;
	int yield;
	void* reVal;
	int join;
	struct threadNode* next;
	struct timeval enqueue_time;
	struct timeval start_time;
	struct timeval finish_time;
} threadNode;


typedef struct Tqueue
{
	threadNode* top;
	threadNode* back;
	int count;
}Tqueue;

typedef struct record {
	worker_t threadID; 
	void* reVal;
	struct record* next;
} record;

/* mutex struct definition */
typedef struct worker_mutex_t {
	int flag;
} worker_mutex_t;
/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
