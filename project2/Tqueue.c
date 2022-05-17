#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void* thread_function(void *ignoredInThisExample)
{
    char *a = malloc(10);
    strcpy(a,"hello world");
    pthread_exit((void*)a);
}
int main()
{
    pthread_t thread_id;
    char *b;

    pthread_create (&thread_id, NULL,&thread_function, NULL);

    pthread_join(thread_id,(void**)&b); //here we are reciving one pointer 
                                        //value so to use that we need double pointer 
    printf("b is %s\n",b); 
    free(b); // lets free the memory

}