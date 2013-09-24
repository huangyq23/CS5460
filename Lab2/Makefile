CC=mpicc
CFLAGS=-I. -L. -std=c99
OBJ = mw_api.o test.o 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o test
