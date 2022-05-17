CC=gcc
CFLAGS= -Dstatic= -Dmain=test_main -D"printf(...)"= -D"puts(...)"=

testfile=bit-shifting
tester=autograder

OBJ= $(tester).o $(testfile).o

LARGE_BITMAP=


$(testfile).o: $(testfile).c
	$(CC) $(CFLAGS) -c -o $@ $^

$(tester).o: $(tester).c
	@$(CC) -c -o $@ $^ $(LARGE_BITMAP)

autograder: $(OBJ)
	@$(CC) -o $@ $^ -lm

test1: autograder

test2: LARGE_BITMAP += -DBITMAP_SIZE=8 -DMY_ADDR=3922544704 -DTEST_NUM=2
test2: autograder

clean: 
	@rm -f *.o autograder
