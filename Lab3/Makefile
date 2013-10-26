CC=mpicc
LIBS=-lgmp
CFLAGS=-I. -L. -std=gnu99

test_worker_f: mw_api_worker_failure_robust.o test1.o
	$(CC) -o $@ $^ $(CFLAGS)
test_master_f: mw_api_worker_master_failure_robust.o test2.o
	$(CC) -o $@ $^ $(CFLAGS)
parallel_kmp:  mw_api_worker_failure_robust.o parallel_kmp.o
	$(CC) -o $@ $^ $(CFLAGS)
parallel_kmp_master_robust:  mw_api_worker_master_failure_robust.o parallel_kmp_master_robust.o
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

