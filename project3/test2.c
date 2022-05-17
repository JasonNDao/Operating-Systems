#define SIZE 5
#define ARRAY_SIZE 400
int main() {

    printf("Allocating three arrays of %d bytes\n", ARRAY_SIZE);
    int s[4094];
    for (int i=0;i<4094;i++){
        s[i]='a';
        //printf("%d", s[i]);
    }
    int t[4098];
    for (int i=0;i<4098;i++){
        t[i]='a';
        //printf("%d", s[i]);
    }
    void *mm = t_malloc(2);
    void *oo = t_malloc(1);
    t_free(mm,1);
    void *a = t_malloc(5010);
    void *me = t_malloc(1);
    printf("Addresses of the allocations: %x", (unsigned int) me);
    for (int index2=0;index2<physical;index2++){   //find physical page in bitmap and  mark as used as well as mark it in page table
        int offset = index2%8;
        int idx = index2/8;
        if ((1&(pBitmap[idx] >> (offset)))==1){
            //printf("%d\n", offset);
        }
    }
    printf("\n");
    int i=0;
    int address_a = 0;
    int y=99;
    char k;
    put_value((void *)a, &t, 4098);
    for (i = 0; i < 4094; i++) {
        address_a = (unsigned int)a + (i *sizeof(char));
        put_value((void *)address_a, (s+i), sizeof(char));
    } 
    put_value((void *)address_a+1, &y, sizeof(int));
    for (i = 0; i < 4094; i++) {
        address_a = (unsigned int)a + (i *sizeof(char));
        get_value((void *)address_a, &k, sizeof(char));
        //printf("%c\n", k);
    } 
    put_value((void *)address_a+1, &y, sizeof(int));
    printf("%d\n", y);
    t_free(a, 5010);
    for (int index2=0;index2<physical;index2++){   //find physical page in bitmap and  mark as used as well as mark it in page table
        int offset = index2%8;
        int idx = index2/8;
        if ((1&(pBitmap[idx] >> (offset)))==1){
            printf("%d\n", offset);
        }
    }
    return 0;

}