ROOT = ../..

include $(ROOT)/common/Makefile.common

ALGO_O = herlihy.o
ALGO_C = herlihy.c

BINS = "$(BINDIR)/lb-sl_herlihy$(BIN_SUFFIX)"
PARTITION = "$(BINDIR)/lb-sl_herlihy_part$(BIN_SUFFIX)"

PROF = $(ROOT)/src

.PHONY:	all clean

all:	main main_partition

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

htlock.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/htlock.o $(PROF)/htlock.c

clh.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/clh.o $(PROF)/clh.c

queue.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/queue.o $(PROF)/queue.c

skiplist-lock.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist-lock.o skiplist-lock.c

$(ALGO_O): skiplist-lock.h 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/$(ALGO_O) $(ALGO_C)

intset.o: skiplist-lock.h optimistic.h htlock.o clh.o
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: skiplist-lock.h optimistic.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

test_partition.o: skiplist-lock.h optimistic.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test_partition.o test_partition.c

main: measurements.o ssalloc.o skiplist-lock.o $(ALGO_O) intset.o test.o clh.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/htlock.o $(BUILDIR)/clh.o $(BUILDIR)/skiplist-lock.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

main_partition: measurements.o ssalloc.o skiplist-lock.o $(ALGO_O) intset.o queue.o test_partition.o clh.o
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/htlock.o $(BUILDIR)/clh.o $(BUILDIR)/skiplist-lock.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/queue.o $(BUILDIR)/intset.o $(BUILDIR)/test_partition.o -o $(PARTITION) $(LDFLAGS)

clean:
	-rm -f $(BINS) $(PARTITION) *.o
