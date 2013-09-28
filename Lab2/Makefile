CC=mpicc
LIBS=-lgmp
CFLAGS=-I. -L. -std=gnu99

all: test largeintdiv parallel_kmp

test: mw_api.o test.o
	$(CC) -o test $^ $(CFLAGS)

largeintdiv: mw_api.o largeintdiv.o
	$(CC) -o largeintdiv $^ $(CFLAGS) $(LIBS)

parallel_kmp:  mw_api.o parallel_kmp.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o test largeintdiv
