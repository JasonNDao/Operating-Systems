// File:	worker.c

// List all group member's name: Donghui Li, Jason Dao
// username of iLab:  jnd88@vi.cs.rutgers.edu
// iLab Server: vi.cs.rutgers.edu
#include "worker.h"


Tqueue* mlfq[LEVELS];
record* front;
ucontext_t scheduler;
ucontext_t main2; 
threadNode* currentThread;
threadNode* main3;
struct itimerval it_val;
struct itimerval it_val2;
struct sigaction sa;

double totalResponseTime = 0;
double totalTurnaroundTime = 0;
int completedJobs = 0;
int first = 1;
unsigned int num = 0;
int currentLevel=0;
int flag=0;
int jflag = 0;
int flag9=0;
int flag10=1;
int globaltime=0;
int totalsize=0;

static void schedule();
threadNode* peek (Tqueue* queue);
void enqueue (Tqueue* queue, threadNode* x);
#ifndef MLFQ
	int choose=0;
#else
    int choose=1;
#endif
static void sched_mlfq();
static void sched_rr();
void NextThread (Tqueue* queue);
threadNode * pop (Tqueue* queue);
void disable();
void enable();
Tqueue * initQ();
double averageResponse();
double averageTurnaround();
void time_diff(struct timeval start, struct timeval stop, struct timeval response);

void functionA () {
	if (choose==0){
		printf("Average Turnaround Time: %f microseconds\n", averageTurnaround());
		printf("Average Response Time: %f microseconds\n", averageResponse());
	}
   	free(scheduler.uc_stack.ss_sp);
	free(main3->TCB->context.uc_stack.ss_sp);
	free(main3->TCB);
	free(main3);
   	for (int i=0; i<LEVELS;i++){
		free(mlfq[i]);
	}
	record *next2;
	record *next1=front;
	while (next1!=NULL){
		next2=next1;
		next1=next1->next;
		free(next2);
	}
}

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
       // - create Thread Control Block (TCB)
       // - create and initialize the context of this worker thread   
       // - allocate space of stack for this thread to run
	   if (first){	 
		   for (int i=0;i<LEVELS;i++) {
			   mlfq[i]=initQ();
		   } 
		   getcontext(&main2);
		   if (first) {
			   struct TCB* M = (TCB*)malloc(sizeof(TCB));
				M->status = READY;
				M->threadID = 4294967295;
				(M->context) = main2;
				M->context.uc_link = atexit(functionA);
				M->context.uc_stack.ss_sp = malloc(SIGSTKSZ);
				M->priority = 0;  
				M->stack = M->context.uc_stack.ss_sp; 
				M->context.uc_stack.ss_size = SIGSTKSZ;   
				M->context.uc_stack.ss_flags = 0;
				if ( M->context.uc_stack.ss_sp == 0 ){
					perror( "malloc: Could not allocate stack" );
					exit( 1 );
				}
				threadNode* M1 = (threadNode*)malloc(sizeof(threadNode));
				M1->TCB = M;
				M1->status = M1->TCB->status;
				M1->next = NULL;
				M1->threadID=4294967295;
				M1->yield=0;
				M1->start=0;
			    M1->join =0;
				main3=M1;
				gettimeofday(&M1->enqueue_time, NULL);
				enqueue(mlfq[0], M1);
				totalsize++;
		   }
		   else return 0;
	   }
	   else {
		   disable();
	   }
	
		struct TCB* temp = (TCB*)malloc(sizeof(TCB));
		temp->status = READY;
		temp->threadID = *thread;
		*thread=num++;
		getcontext(&temp->context);
		temp->context.uc_link = 0;
		temp->context.uc_stack.ss_sp = malloc(SIGSTKSZ);
		temp->priority = 0;  
		temp->stack = temp->context.uc_stack.ss_sp; // check
		temp->context.uc_stack.ss_size = SIGSTKSZ;   
		temp->context.uc_stack.ss_flags = 0;        
		if ( temp->context.uc_stack.ss_sp == 0 ){
			perror( "malloc: Could not allocate stack" );
			exit(1);
		}
		if (arg==NULL){
			makecontext(&temp->context, (void (*)(void))function, 0);		
		}
		else{
			makecontext(&temp->context, (void (*)(void))function, 1 ,arg);	
		}
		threadNode* M2 = (threadNode*)malloc(sizeof(threadNode));
		M2->TCB = temp;
		M2->status = M2->TCB->status;
		M2->next = NULL;
		M2->threadID=*thread;
		M2->yield=0;
		M2->start=0;
		M2->join =0;
		gettimeofday(&M2->enqueue_time, NULL);
		/*printf("seconds : %ld\tmicro seconds : %ld\n",
   		M2->enqueue_time.tv_sec, M2->enqueue_time.tv_usec);*/
		enqueue(mlfq[0], M2);
		totalsize++;
		if(first == 1){
			getcontext(&scheduler);
			scheduler.uc_link = 0;
			scheduler.uc_stack.ss_sp = malloc(SIGSTKSZ);  
			scheduler.uc_stack.ss_size = SIGSTKSZ;   
			scheduler.uc_stack.ss_flags = 0;  
			makecontext(&scheduler, schedule, 0);
			setcontext(&scheduler);
		}
		else {
			enable();
		}
	   
    return 0;
}

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context
	disable();
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	if (currentThread->yield==0 || choose==0){
		globaltime += ((QUANTUM+(QUANTUM2*currentLevel))*1000 - it_val.it_value.tv_usec);
	}
	else{
		globaltime += (currentThread->yield - it_val.it_value.tv_usec);
	}
	currentThread->TCB->status = READY;
	currentThread->status = READY;
	currentThread->yield = it_val.it_value.tv_usec;
	if (jflag){
		currentThread->TCB->priority = LEVELS - 1;
		currentThread->yield = 0;
		jflag = 0;
	}
	swapcontext(&currentThread->TCB->context, &scheduler);
	return 0;
};

int worker_block() {
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context
	disable();
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	if (currentThread->yield==0 || choose==0){
		globaltime += ((QUANTUM+(QUANTUM2*currentLevel))*1000 - it_val.it_value.tv_usec);
	}
	else{
		globaltime += (currentThread->yield - it_val.it_value.tv_usec);
	}
	currentThread->TCB->status = BLOCKED;
	currentThread->status = BLOCKED;
	currentThread->yield = it_val.it_value.tv_usec;
	swapcontext(&currentThread->TCB->context, &scheduler);
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
	disable();
	gettimeofday(&currentThread->finish_time, NULL);
	totalsize--;
	if (currentThread->yield==0  || choose==0){
		globaltime += ((QUANTUM+(QUANTUM2*currentLevel))*1000 - it_val.it_value.tv_usec);
	}
	else{
		globaltime += (currentThread->yield - it_val.it_value.tv_usec);
	}
	completedJobs++;
	time_diff(currentThread->enqueue_time, currentThread->start_time, currentThread->finish_time);
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	currentThread->status = TERMINATED;
	free(currentThread->TCB->context.uc_stack.ss_sp);
	free(currentThread->TCB);
	if (value_ptr != NULL) currentThread->reVal = value_ptr;
	record* save = malloc(sizeof(record));
	save->threadID = currentThread->threadID;
	save->reVal = currentThread->reVal;
	save->next = front;
	front = save;
	setcontext(&scheduler);
}


/* Wait for thread termination */
//need more work
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
	threadNode* tnode;
	threadNode *prev;
	disable();
	for(int i = 0; i < LEVELS; i++){
		tnode = mlfq[i]->top;
		while (tnode!=NULL && tnode->threadID!=thread){
			//prev=tnode;
			tnode=tnode->next;
		}
		if (tnode != NULL) break;
	}

	if (tnode != NULL){
		tnode->join = 1;
		enable();
		while (tnode->status != TERMINATED){
			jflag = 1;
			worker_yield();
		}
		disable();
		for(int i = 0; i < LEVELS; i++){
			tnode = mlfq[i]->top;
			while (tnode!=NULL && tnode->threadID!=thread){
				prev=tnode;
				tnode=tnode->next;
			}
			if (tnode != NULL) {
				if (mlfq[i]->count==1){
					mlfq[i]->back=NULL;
					mlfq[i]->top=NULL;
				}
				else if (tnode==mlfq[i]->top){
					mlfq[i]->top=tnode->next;
				}
				else if (tnode==mlfq[i]->back){
					mlfq[i]->back=prev;
					prev->next=NULL;
				}
				else{
					prev->next=tnode->next;
				}
				free(tnode);
				mlfq[i]->count--;
				break;
			}
		}
		enable();
	}
	else enable();
	
	if (value_ptr != NULL) {
		record* ptr = front;
		record* prev = NULL;

		disable();
		while (ptr->threadID != thread && ptr != NULL){
			prev = ptr;
			ptr = ptr->next;
		}
		enable();

		if (ptr != NULL){
			*value_ptr = (void *)ptr->reVal;
			if (prev == NULL) front = front->next;
			else prev->next = ptr->next;
			free(ptr);
		}
		//else value_ptr = (NULL);
	}

	return 0;
}

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex
	mutex->flag=0;
	return 0;
}

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
        //_atomic_and_set()
        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread
		disable();
		while ((__atomic_test_and_set(&mutex->flag, 1) == 1) && mutex != NULL){
			//worker_yield();
			worker_block();
		}
		//mutex->flag = 1;
		enable();
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.
	disable();
	if (mutex != NULL) mutex->flag = 0;
	for (int i=0;i<LEVELS;i++){
		threadNode * temp=mlfq[i]->top;
		while (temp!=NULL){
			if(temp->status==BLOCKED){
				temp->TCB->status=READY;
				temp->status=READY;
			}
			temp=temp->next;
			
		}
	}
	enable();
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init
	disable();
	mutex->flag = 0;
	enable();
	return 0;
};

void timerHandler(){
	flag10++;
	//printf("Out of time\n");
	currentThread->yield=0;
	if (currentThread->TCB->priority != LEVELS-1) currentThread->TCB->priority += 1;
    swapcontext(&currentThread->TCB->context, &scheduler);
    return;
}

void timer(){
		memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = timerHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGALRM, &sa, NULL);
		if (currentThread->yield!=0 && choose==1){
			it_val.it_value.tv_sec =  0;
        	it_val.it_value.tv_usec =  (currentThread->yield) ;
			currentThread->yield=0;
		}
		else{
			it_val.it_value.tv_sec =  (QUANTUM+(QUANTUM2*currentLevel))/1000;
        	it_val.it_value.tv_usec =  ((QUANTUM+(QUANTUM2*currentLevel))*1000) % 1000000;
		}		
		it_val.it_interval.tv_sec = 0; 
		it_val.it_interval.tv_usec = 0;
        if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
        	exit(1);
        }
}

void disable(){
	it_val2.it_value.tv_sec = 0;
    it_val2.it_value.tv_usec = 0;	
    it_val2.it_interval.tv_sec = 0;
    it_val2.it_interval.tv_usec = 0;

	if (setitimer(ITIMER_REAL,&it_val2, &it_val) == -1) {
        printf("error calling setitimer()");
        exit(1);
    }
	return;
}

void enable(){
	//printf("remaing %ld, %ld \n",it_val.it_value.tv_sec, it_val.it_value.tv_usec);
	//printf("remaining %ld \n", it_val.it_value.tv_usec);
	if (it_val.it_value.tv_sec == 0 && it_val.it_value.tv_usec == 0) timerHandler();
	else{
		if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
			printf("error calling setitimer()");
			exit(1);
		}
	}
	return;
}

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function
	// - invoke scheduling algorithms according to the policy (RR or MLFQ)
	if (choose==0 || LEVELS==1){
		sched_rr();
	}
	else{
		sched_mlfq();
	}
}


void time_diff(struct timeval start, struct timeval response, struct timeval stop){
	totalTurnaroundTime += (double)((stop.tv_usec - start.tv_usec))+ (double)((stop.tv_sec - start.tv_sec)*1000000);
	totalResponseTime += (double)((response.tv_usec - start.tv_usec))+ (double)((response.tv_sec - start.tv_sec)*1000000);
}

/* Round-robin (RR) scheduling algorithm */
static void sched_rr() {
	// - your own implementation of RR
	// (feel free to modify arguments and return types)
	if (first==1){
		mlfq[0]->top->TCB->status=SCHEDULED;
		mlfq[0]->top->status = SCHEDULED;
		currentThread=mlfq[0]->top;
		currentThread->start = 1;
		first=0;
		timer();
		gettimeofday(&mlfq[0]->top->start_time, NULL);
		setcontext(&mlfq[0]->top->TCB->context);
	}
	if (currentThread->status == TERMINATED && currentThread->join == 0) {
		pop(mlfq[0]);
		free(currentThread);
	}
	else{
		if (currentThread->status == SCHEDULED) {
			currentThread->TCB->status = READY;
			currentThread->status = READY;
		}
		NextThread(mlfq[0]);
	}
	
	int loopCount = 1;
	while (loopCount <= mlfq[0]->count && mlfq[0]->top->status != READY){
		NextThread(mlfq[0]);
		loopCount++;
	}

	if (loopCount > mlfq[0]->count) {
		printf("No runable thread, potential deadlock, exit!\n");
		exit(1);
	}

	mlfq[0]->top->TCB->status=SCHEDULED;
	mlfq[0]->top->status = SCHEDULED;
	currentThread = mlfq[0]->top;
	if (currentThread->start == 0) {
		gettimeofday(&currentThread->start_time, NULL);
		currentThread->start = 1;
	}
	timer();
	setcontext(&mlfq[0]->top->TCB->context);
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)
	if (first==1){
		currentLevel=0;
		mlfq[0]->top->TCB->status=SCHEDULED;
		mlfq[0]->top->status = SCHEDULED;
		currentThread=mlfq[0]->top;
		currentThread->start=1;
		first=0;
		timer();
		gettimeofday(&mlfq[0]->top->start_time, NULL);
		setcontext(&mlfq[0]->top->TCB->context);
	}
	
	if (currentThread->status == TERMINATED && currentThread->join == 0) {
		pop(mlfq[currentLevel]);
		free(currentThread);
	}
	else{
		if (currentThread->status == SCHEDULED) {
			currentThread->TCB->status = READY;
			currentThread->status = READY;
		}
		if (currentThread->status != TERMINATED){
			threadNode* temp7 = pop(mlfq[currentLevel]);
			enqueue(mlfq[temp7->TCB->priority], temp7);
		}
	}
	
	
	if (globaltime >= S*1000){
		for(int i = 1; i < LEVELS; i++){
			while (mlfq[i]->count!=0){
				threadNode *temp2=pop(mlfq[i]);
				if (temp2->status != TERMINATED) temp2->TCB->priority=0;
				enqueue(mlfq[0], temp2);
			}
		}
		globaltime=0;
	}
	int flag8=0;
	for (int k=0;k<LEVELS;k++){
		int loopCount=0;
		while (mlfq[k]->count!=0 && loopCount!=mlfq[k]->count){
			if (mlfq[k]->top->status==READY){
				flag8=1;
				break;
			}
			NextThread(mlfq[k]);
			loopCount++;
		}	
		if (flag8==1){
			currentLevel=k;
			break;
		}
	}

	mlfq[currentLevel]->top->TCB->status = SCHEDULED;
	mlfq[currentLevel]->top->status = SCHEDULED;
	currentThread = mlfq[currentLevel]->top;
	timer();
	setcontext(&currentThread->TCB->context);
}

Tqueue * initQ(){
    Tqueue * temp = (Tqueue *) malloc(sizeof(Tqueue));
    temp->count = 0;
    temp->top = NULL;
    temp->back = NULL;
    return temp;
}

int isEmpty (Tqueue* queue){
    if (queue == NULL) return 1;
    if (queue->count == 0) return 1;
    else return 0;
}

void enqueue (Tqueue* queue, threadNode* x){
    if (queue == NULL) {
		return;
	}
    if (queue->count == 0){
        queue->top = x;
        queue->back = x;
    }
    else{
        (queue->back)->next = x;
        queue->back = x;
    }
    x->next = NULL;
    queue->count++;
}

threadNode* pop (Tqueue* queue){
    if (isEmpty(queue)) return NULL;
    threadNode* temp = queue->top;
    queue->top = (queue->top)->next;
    if (queue ->count == 1) queue->back = NULL;
    queue->count--;
    return temp;
}

void NextThread (Tqueue* queue) {
	if (isEmpty(queue) == 0){
		threadNode* temp = pop(queue);
		enqueue(queue, temp);
	}
}

threadNode* peek (Tqueue* queue){
    if (isEmpty(queue)) return NULL;
    return queue->top;
}

double averageResponse(){
	return (double) totalResponseTime/(double)completedJobs;
}

double averageTurnaround() {
	return (double) totalTurnaroundTime/(double)completedJobs;
}
