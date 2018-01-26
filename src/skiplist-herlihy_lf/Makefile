ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/lf-sl_herlihy
PARTITION = $(BINDIR)/lf-sl_herlihy_part
PROF = $(ROOT)/src

CFLAGS += -O2

.PHONY:	all clean

all:	main main_partition

ALGO_O = herlihy.o
ALGO_C = herlihy.c
measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

queue.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/queue.o $(PROF)/queue.c

skiplist.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist.o skiplist.c

$(ALGO_O): skiplist.h 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/$(ALGO_O) $(ALGO_C)

intset.o: skiplist.h herlihy.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: skiplist.h herlihy.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

test_partition.o: skiplist.h herlihy.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test_partition.o test_partition.c

main: measurements.o ssalloc.o skiplist.o $(ALGO_O) intset.o test.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

main_partition: measurements.o ssalloc.o skiplist.o $(ALGO_O) intset.o queue.o test_partition.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/queue.o $(BUILDIR)/intset.o $(BUILDIR)/test_partition.o -o $(PARTITION) $(LDFLAGS)

clean:
	-rm -f $(BINS) $(PARTITION)
