CC=mpicc
LIBS=-lgmp
CFLAGS=-I. -L. -std=gnu99

all: test largeintdiv

test: mw_api.o test.o
	$(CC) -o test $^ $(CFLAGS)

largeintdiv: mw_api.o largeintdiv.o
	$(CC) -o largeintdiv $^ $(CFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o test largeintdiv