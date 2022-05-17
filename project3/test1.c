#define SIZE 5
#define ARRAY_SIZE 400
int main() {
    void *a = t_malloc(4100000);
    void *b = t_malloc(200000);
    void *c = t_malloc(10);
    void *d = t_malloc(4100000);
    void *e = t_malloc(4100000);
    void *f = t_malloc(5100000);
    printf("Addresses of the allocations: %x, %x, %x, %x", (unsigned long int)a, (int)b, (int)c, (unsigned long int)e);
    printf("Freeing the allocations!\n");
    t_free(a, 4100000);
    t_free(b, 200000);
    t_free(c, 10);
    t_free(d, 4100000);
    t_free(e, 4100000);
    t_free(f, 5100000);
    
    void *kl = t_malloc(ARRAY_SIZE);
    t_free(kl, ARRAY_SIZE+1);
    
    return 0;

}