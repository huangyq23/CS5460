CC=mpicc
CFLAGS=-I. -L. -std=gnu99
OBJ = mw_api.o test.o 
OBJ1 = mw_api.o parallel_kmp.o
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

parallel_kmp: $(OBJ1)
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	rm -rf *.o test
