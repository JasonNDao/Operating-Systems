#include "my_vm.h"

int first=0;
int lockfirst=0;

void *ptr;
void *currptr;
pde_t * top;

int bitOffset=-1;
int bitLevelOne=0;
int bitLevelTwo=0;

unsigned long size1=0;

char *pBitmap;
char *vBitmap;
unsigned long physical=0;
unsigned long virtual=0;
int newpde=0;

struct tlb cache[TLB_ENTRIES];
int tlbsize=0;
int totallookups=0;
int hit=0;
int miss=0;

int size = 0;
struct Node *head;

pthread_mutex_t lock1=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock3=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock4=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock5=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock6=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock7=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock8=PTHREAD_MUTEX_INITIALIZER;

void initPageTable();

void insert(unsigned long int number, unsigned long int v){
    pthread_mutex_lock(&lock5);
    struct Node *newnode = (struct Node *)malloc(sizeof(struct Node));
    newnode->data = number;
    newnode->next = NULL;
    newnode->virtual=v;
    if (size == 0){
        head = newnode;
        newnode->next = NULL;
    }
    else
    {
        struct Node *previous = NULL;
        struct Node *pointer = head;
        while (pointer != NULL && ((pointer->virtual) < (newnode->virtual))){
            previous = pointer;
            pointer = pointer->next;
        }
        if (previous == NULL){
            newnode->next = head;
            head = newnode;
        }
        else if (pointer == NULL){
            previous->next = newnode;
        }
        else{
            newnode->next = previous->next;
            previous->next = newnode;
        }
    }
    size++;
    pthread_mutex_unlock(&lock5);
}
unsigned long int value(unsigned long int v){
    pthread_mutex_lock(&lock5);
    if (head != NULL){
        struct Node *previous = NULL;
        struct Node *pointer = head;
        while (pointer!=NULL){
            if (pointer->virtual==v){
                pthread_mutex_unlock(&lock5);
                return pointer->data;
            }
            previous=pointer;
            pointer = pointer->next;
        }
    }
    pthread_mutex_unlock(&lock5);
    return 0;
}
unsigned long int value2(unsigned long int v){
    pthread_mutex_lock(&lock5);
    if (head != NULL){
        struct Node *previous = NULL;
        struct Node *pointer = head;
        while (pointer!=NULL){
            unsigned long int hi=pointer->virtual+pointer->data;
            if (v>=pointer->virtual && v<=hi){
                pthread_mutex_unlock(&lock5);
                return pointer->virtual;
            }
            previous=pointer;
            pointer = pointer->next;
        }
    }
    pthread_mutex_unlock(&lock5);
    return 0;
}
void delete (unsigned long int v){
    pthread_mutex_lock(&lock5);
    struct Node *previous = NULL;
    struct Node *pointer = head;
    while (pointer != NULL && ((pointer->data) < v)){
        previous = pointer;
        pointer = pointer->next;
    }
    if (previous==NULL && pointer != NULL && pointer->data==v){
        head=pointer->next;
        pointer->next=NULL;
        free(pointer);
        size--;
        pthread_mutex_unlock(&lock5);
        return;
    }
    if(pointer != NULL && pointer->data==v){
        previous->next=pointer->next;
        pointer->next=NULL;
        free(pointer);
        size--;
        pthread_mutex_unlock(&lock5);
        return;
    }
    pthread_mutex_unlock(&lock5);
}

void freeList(struct Node* top)
{
   struct Node* temp;

   while (top != NULL)
    {
       temp = top;
       top = top->next;
       free(temp);
    }

}

void freeEverything(){
    print_TLB_missrate();
    free(ptr);
    free(pBitmap);
    free(vBitmap);
    freeList(head);
}

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    ptr=malloc(MEMSIZE);
    memset(ptr,0, MEMSIZE);
    
    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them
    
    physical=MEMSIZE/PGSIZE;  //p1 p2 p3 p4
    virtual=MAX_MEMSIZE/PGSIZE;   //pd0pt0offset0 pd0pt0offset1 pd0pt0offset2
    if(physical %8 !=0){
        pBitmap = (char *)malloc(physical/8+1); //check lengths are correct 
        memset(pBitmap,0,physical/8+1);
    }
    else{
        pBitmap = (char *)malloc(physical/8);
        memset(pBitmap,0,physical/8);
    }
     if (virtual%8!=0){
        vBitmap = (char *)malloc(virtual/8+1);
        memset(vBitmap,0,virtual/8+1);
    }
    else{
        vBitmap = (char *)malloc(virtual/8);
        memset(vBitmap,0,virtual/8);
    }
    atexit(freeEverything);
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
int
add_TLB(void *va, void *pa)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    pthread_mutex_lock(&lock3);
    unsigned long int index=(unsigned long int)va % TLB_ENTRIES;
    cache[index].virtual=(unsigned long int)va;
    cache[index].physical=(unsigned long int)pa;
    pthread_mutex_unlock(&lock3);
    return 1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
pte_t *
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */
    pthread_mutex_lock(&lock3);
    totallookups++;
    unsigned long int indexy=(unsigned long int)va % TLB_ENTRIES;
    if (cache[indexy].virtual==(unsigned long int)va){
        hit++;
        pthread_mutex_unlock(&lock3);
        return &(cache[indexy].physical);
    }
    miss++;
    pthread_mutex_unlock(&lock3);
    return NULL;


}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;	
    miss_rate=(double)miss/(double)totallookups;

    /*Part 2 Code here to calculate and print the TLB miss rate*/

    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}

void delete_TLB(void *va) {

    /* Part 2: TLB lookup code here */
    pthread_mutex_lock(&lock3);
    unsigned long int index=(unsigned long int)va%TLB_ENTRIES;
    if (cache[index].virtual==(unsigned long int)va){
        cache[index].virtual=0;
        cache[index].physical=0;
    }
    pthread_mutex_unlock(&lock3);
    return;


}


/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {  //for free, get value and put value
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */
    pthread_mutex_lock(&lock2);
    char str1[32];
    sprintf(str1, "%ld", (unsigned long int)va);
    unsigned long int number1=strtol(str1,NULL, 10);
    unsigned long int l=number1>>(bitOffset);
    pte_t * check=check_TLB((void *)l);
    if (check!=NULL){
        pthread_mutex_unlock(&lock2);
        return check;
    }
    int i=0;
    for (i=0;i<(round(pow(2,bitLevelOne)));i++){         
        if (top[i]==*pgdir){
            break;
        }
    }
    char str[32];
    sprintf(str, "%ld", (unsigned long int)va);
    unsigned long int number=strtol(str,NULL, 10);
    number=number>>bitOffset;                                                          
    unsigned long int mask = (1 << bitLevelTwo) - 1;                                                    
    number = number & mask;
    //If translation not successful, then return NULL
    if (((pte_t *)top[i])[number]==0){
        pthread_mutex_unlock(&lock2);
        return NULL;
    }
    //return NULL;
    add_TLB((void *)l, (void *)(((pte_t *)top[i])[number]));
    pte_t * ret=&((((pte_t *)top[i])[number]));
    pthread_mutex_unlock(&lock2);
    return ret; 
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)  //IF OUT OF PDE, new one
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */
    pthread_mutex_lock(&lock2);
    int i=0;
    for (i=0;i<(round(pow(2,bitLevelOne)));i++){         
        if (top[i]==*pgdir){
            break;
        }
    }
    char str[32];
    sprintf(str, "%ld", (unsigned long int)va);
    unsigned long int number=strtol(str,NULL, 10);
    number=number>>bitOffset;                                                          
    unsigned long int mask = (1 << bitLevelTwo) - 1;                                                    
    number = number & mask;
    char str1[32];
    sprintf(str1, "%ld", (unsigned long int)va);
    unsigned long int number1=strtol(str1,NULL, 10);
    unsigned long int l=number1>>(bitOffset);
    if (((pte_t *)top[i])[number]==0){
        ((pte_t *)top[i])[number]=(unsigned long int)pa;
        //add_TLB((void *)l, (void *)(((pte_t *)top[i])[number]));
        pthread_mutex_unlock(&lock2);
        return 1;
    }
    pthread_mutex_unlock(&lock2);
    return -1;
}


/*Function that gets the next available page
*/
void *get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
    pthread_mutex_lock(&lock1);
    int flag=0;
    int flag2=0;  
    int index2=0;
    int index=0;
    int temp=0;
     for (index=0;index<virtual;index++){    //find virtual page in bitmap     //OUT OF VIRTUAL PAGES QUIT
        int offset = index%8;
	    int idx = index/8;
	    if ((1&(vBitmap[idx] >> (offset)))==0){
            if (index>virtual-num_pages){
                printf("Not enough virtual memory");
                pthread_mutex_unlock(&lock1);
                return NULL;
            }
            for (int j=0;j<num_pages;j++){
                offset = (index+j)%8;
	            idx =(index+j)/8;
                if ((1&(vBitmap[idx] >> (offset)))==1){
                    flag2=1;
                }
            }
            if (flag2==1){
                flag2=0;
                continue;
            }
            pthread_mutex_lock(&lock2);
            for (int jl=0;jl<num_pages;jl++){ 
                if ((index+jl)/(int)round(pow(2,bitLevelTwo))>newpde && newpde<(int)round(pow(2,bitLevelOne))){
                    pte_t top28[(int)round(pow(2,bitLevelTwo))];
                    memset(top28, 0, round(pow(2,bitLevelTwo))*sizeof(pte_t));
                    //mount to currptr
                    for (int k=0;k<round(pow(2,bitLevelTwo));k++){
                        top28[k]=0;
                    }
                     for (int jindex=0;jindex<virtual;jindex++){    //find virtual page in bitmap
                        int offset8 = jindex%8;
                        int idx8 = jindex/8;
                        if ((1&(vBitmap[idx8] >> (offset8)))==0){
                            vBitmap[idx8] |= 1 << (offset8);
                            break;
                        }
                        
                    }
                    for (index2=0;index2<physical;index2++){   
                        int offset7 = index2%8;
                        int idx7 = index2/8;
                        if ((1&(pBitmap[idx7] >> (offset7)))==0){
                            pBitmap[idx7] |= 1 << (offset7);
                            break;
                        }
                    }
                    memcpy(ptr+(index2*PGSIZE),top28,round(pow(2,bitLevelTwo))*sizeof(pte_t));
                    top[(index+jl)/(int)round(pow(2,bitLevelTwo))]=(unsigned long int)ptr+(index2*PGSIZE); 

                  //  pBitmap[0] |= 1 << (1);
                    flag2=1;
                    newpde=(index+jl)/(int)round(pow(2,bitLevelTwo));
                    break;
                }
                
            }
            pthread_mutex_unlock(&lock2);
            if (flag2==1){
                index=0;
                flag2=0;
                continue;
            }
            break;
        }
    }
   //see if enough physical
    for (index2=0;index2<physical;index2++){   
        int offset = index2%8;
        int idx = index2/8;
        if ((1&(pBitmap[idx] >> (offset)))==0){
            temp++;
            if (temp==num_pages){
                break;
            }
        }
    }
    if (temp!=num_pages){ //not enough physical pages
        pthread_mutex_unlock(&lock1);
        printf("Not enough physical memory!\n");
        return NULL;
    } 
    //assign virtual address
    //printf("Index2:%d\t", index2);
    //printf("%d\t", num_pages);
    unsigned long int x3;
    for (int p=0;p<num_pages;p++){
        int pd=(index+p)/(int)round(pow(2,bitLevelTwo));   //change to bit level2
        int ptt=(index+p)%(int)round(pow(2,bitLevelTwo));  //change to bit level2
        unsigned long int binary_number, hexadecimal_number = 0, i = 1; 
        binary_number=(((pd << bitLevelTwo) | ptt)<<bitOffset); 
        char hexval[32];
        sprintf(hexval,"%0lx",binary_number);
        //printf("%ld\n", binary_number);
        unsigned long int x4 = strtol(hexval, NULL, 16);
        if (p==0){
            x3=x4;
        }
        for (index2=0;index2<physical;index2++){   //find physical page in bitmap and  mark as used as well as mark it in page table
            int offset = index2%8;
            int idx = index2/8;
            if ((1&(pBitmap[idx] >> (offset)))==0){
                if (page_map(&top[pd],(void *)x4,ptr+(index2*PGSIZE))==-1){ 
                    pthread_mutex_unlock(&lock1);
                    return NULL;
                }
                pBitmap[idx] |= 1 << (offset);
                break;
            }
        }
    }
    for (index=0;index<virtual;index++){    //find virtual page in bitmap
        int offset = index%8;
	    int idx = index/8;
	    if ((1&(vBitmap[idx] >> (offset)))==0){
            if (index>virtual-num_pages){
                pthread_mutex_unlock(&lock1);
                return NULL;
            }
            for (int j=0;j<num_pages;j++){
                offset = (index+j)%8;
	            idx =(index+j)/8;
                if ((1&(vBitmap[idx] >> (offset)))==1){
                    flag2=1;
                }
            }
            if (flag2==1){
                flag2=0;
                continue;
            }
            for (int j=0;j<num_pages;j++){
                offset = (index+j)%8;
	            idx =(index+j)/8;
                vBitmap[idx] |= 1 << (offset);
            }
            break;
        }
    }
    pthread_mutex_unlock(&lock1);
    return (void *)x3;                                                     
    
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *t_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */
    
    pthread_mutex_lock(&lock4);
    if (first==0){
        set_physical_mem();
        first=1;
        currptr=ptr;
        initPageTable(); //need to fix to use tmalloc
    }
    pthread_mutex_unlock(&lock4);
    //set physical and virtual bitmaps
    void * s;
    if (num_bytes%PGSIZE!=0){
        s=get_next_avail(num_bytes/PGSIZE+1);
    }
    else{
        s=get_next_avail(num_bytes/PGSIZE);
    }
    pthread_mutex_lock(&lock1);
    if (s!=NULL){
        insert(num_bytes, (unsigned long int)s);
    }
    pthread_mutex_unlock(&lock1);
    return (void *)s;
    
   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void t_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
    pthread_mutex_lock(&lock1);
    int num_pages=size/PGSIZE;
    if (size%PGSIZE !=0 ){
        num_pages++;
    }
    if (value((unsigned long int)va)==0 || size>value((unsigned long int)va)){
        printf("Accessing memory that was not allocated to you\n");
        pthread_mutex_unlock(&lock1); 
        return;
    }
    void * theHead=va;
    for (int i=0;i<num_pages;i++){
        if (i!=0){
             va=va+(PGSIZE);
        }
        //printf("%d\n", va);
        char str[32];
        sprintf(str, "%ld", (unsigned long int)va);
        //printf("%s\t", str);
        unsigned long int number=strtol(str,NULL, 10);
        //printf("%ld\t", number);

        pde_t * p=&top[(number>>(bitLevelTwo+bitOffset))]; 
        pte_t * ph=translate(p,va);
        if (ph==NULL){
            int index=((number>>(bitLevelTwo+bitOffset))<<bitLevelTwo)|((number>>bitOffset) & ((1 << bitLevelTwo) - 1));
            int offset = index%8;
            int idx = index/8;
            vBitmap[idx] &= ~(1ULL << offset);

            number=number>>bitOffset;                                                          
            unsigned long int mask = (1 << bitLevelTwo) - 1;                                                    
            number = number & mask; 
            pthread_mutex_lock(&lock2);
            (((pte_t *)top[(number>>(bitLevelTwo+bitOffset))])[number])=0;
            pthread_mutex_unlock(&lock2);
            continue;
        }
        memset((void *)*ph, 0, PGSIZE);

        int index=((number>>(bitLevelTwo+bitOffset))<<bitLevelTwo)|((number>>bitOffset) & ((1 << bitLevelTwo) - 1));
        int offset = index%8;
        int idx = index/8;
        vBitmap[idx] &= ~(1ULL << offset);

        index=(unsigned long int)*ph-(unsigned long int) currptr;
        index=index/PGSIZE;
        offset = index%8;
        idx = index/8;
        pBitmap[idx] &= ~(1ULL << offset);

        number=number>>bitOffset;                                                          
        unsigned long int mask = (1 << bitLevelTwo) - 1;                                                    
        number = number & mask; 
        pthread_mutex_lock(&lock2);
        (((pte_t *)top[(number>>(bitLevelTwo+bitOffset))])[number])=0;
        pthread_mutex_unlock(&lock2); 
        delete_TLB(va);

    }
    delete((unsigned long int)theHead);
    pthread_mutex_unlock(&lock1); 
    return;
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */ 
    pthread_mutex_lock(&lock1);
    
    unsigned long int r=value2((unsigned long int)va);
     if (r== 0){
        printf("Trying to access unallocated memory\n");
        pthread_mutex_unlock(&lock1);
        return;
    }
    unsigned long int s=value((unsigned long int)r);
    if (s==0){
        printf("Trying to access unallocated memory\n");
        pthread_mutex_unlock(&lock1);
        return;
    }
    if ((unsigned long)va+size>r+s){
        size=r+s-(unsigned long int)va;
    }
    
    for (int k=0;k<size;k++){
        if (k!=0){
             va+=1;
        }
        char str[32];
        sprintf(str, "%ld", (unsigned long int)va);
        //printf("%s\t", str);
        unsigned long int number=strtol(str,NULL, 10);
        unsigned long int mask = (1 << bitOffset) - 1;                                                                                                       
        unsigned long int number2 = number & mask;
        pde_t * p=&top[(number>>bitLevelTwo+bitOffset)];
        pte_t * ph=translate(p,va);
        if (k==0){
            char str2[32];
            sprintf(str2, "%ld", (unsigned long int)va+size);
            unsigned long int number3=strtol(str2,NULL, 10);
            unsigned long int mask2 = (1 << bitOffset) - 1;                                                                                                       
            unsigned long int number4 = number3 & mask2;
            if (number>>bitOffset==number3>>bitOffset){
                pthread_mutex_lock(&lock2);
                memcpy((void *)*ph+number2,val,size);
                pthread_mutex_unlock(&lock2);
                break;
            }
        }
        if( ph==NULL){
            break;
        }
        pthread_mutex_lock(&lock2);
        memcpy((void *)*ph+number2,val+k,1);
        pthread_mutex_unlock(&lock2);
    }
    pthread_mutex_unlock(&lock1);
    return;



}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */
    pthread_mutex_lock(&lock1);
    unsigned long int r=value2((unsigned long int)va);
     if (r==0){
        printf("Trying to access unallocated memory\n");
        pthread_mutex_unlock(&lock1);
        return;
    }
    unsigned long int s=value(r);
    if (s==0){
        printf("Trying to access unallocated memory\n");
        pthread_mutex_unlock(&lock1);
        return;
    }
    if ((unsigned long)va+size>r+s){
        size=r+s-(unsigned long int)va;
    }     
    for (int k=0;k<size;k++){
        if (k!=0){
             va+=1;
        }
        char str[32];
        sprintf(str, "%ld", (unsigned long int)va);
        //printf("%s\t", str);
        unsigned long int number=strtol(str,NULL, 10);
        unsigned long int mask = (1 << bitOffset) - 1;                                                                                                       
        unsigned long int number2 = number & mask;
        pde_t * p=&top[(number>>bitLevelTwo+bitOffset)];
        pte_t * ph=translate(p,va);
        
        if (k==0){
            char str2[32];
            sprintf(str2, "%ld", (unsigned long int)va+size);
            unsigned long int number3=strtol(str2,NULL, 10);
            unsigned long int mask2 = (1 << bitOffset) - 1;                                                                                                       
            unsigned long int number4 = number3 & mask2;
            if (number>>bitOffset==number3>>bitOffset){
                pthread_mutex_lock(&lock2);
                memcpy(val,(void *)*ph+number2,size);
                pthread_mutex_unlock(&lock2);
                break;
            }
        }
        
        if( ph==NULL){
            break;
        }
        pthread_mutex_lock(&lock2);
        memcpy(val+k,(void *)*ph+number2,1);
        pthread_mutex_unlock(&lock2);
    }
    pthread_mutex_unlock(&lock1);
    return;

}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                // printf("Values at the index: %d, %d, %d, %d, %d\n", 
                //     a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            // printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}

void initPageTable(){ 

    unsigned int temp=PGSIZE;
    while (temp){        
        bitOffset+=1;
        temp >>= 1;
    }
    bitLevelOne=(32-bitOffset)/2; 
    bitLevelTwo=bitLevelOne;
    if (bitLevelOne %2 !=0){
        bitLevelTwo++;
    }
    
    pde_t stuff[(int)round(pow(2,bitLevelOne))];
    memset(stuff, 0, (int)round(pow(2,bitLevelOne))*sizeof(pde_t));
    memcpy(currptr,stuff,(int)round(pow(2,bitLevelOne))*sizeof(pde_t));
    top=currptr;
    vBitmap[0] |= 1 << (0);
    pBitmap[0] |= 1 << (0);
    
    pte_t top2[(int)round(pow(2,bitLevelTwo))];
    memset(top2, 0, (int)round(pow(2,bitLevelTwo))*sizeof(pte_t));
    //mount to currptr
    for (int k=0;k<round(pow(2,bitLevelTwo));k++){
        top2[k]=0;
    } //mount to currptr
    memcpy(currptr+PGSIZE,top2,(int)round(pow(2,bitLevelTwo))*sizeof(pte_t));
    top[0]=(unsigned long int)currptr+PGSIZE;
    vBitmap[0] |= 1 << (1);
    pBitmap[0] |= 1 << (1);
    
    return;
}

