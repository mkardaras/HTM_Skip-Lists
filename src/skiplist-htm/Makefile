ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/htm-sl
PARTITION = $(BINDIR)/htm-sl_part
PROF = $(ROOT)/src

CFLAGS += -msse4.1

.PHONY:	all clean

all:	main main_partition

ALGO_O = key_operations.o
ALGO_C = key_operations.c
measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

queue.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/queue.o $(PROF)/queue.c

skiplist_TM.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist_TM.o skiplist_TM.c

$(ALGO_O): skiplist_TM.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/$(ALGO_O) $(ALGO_C)

test.o: htm_intel.h default_parameters.h skiplist_TM.h key_operations.h 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

test_partition.o: htm_intel.h default_parameters.h skiplist_TM.h key_operations.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test_partition.o test_partition.c

main: measurements.o ssalloc.o skiplist_TM.o $(ALGO_O) test.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist_TM.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

main_partition: measurements.o ssalloc.o skiplist_TM.o $(ALGO_O) queue.o test_partition.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist_TM.o $(BUILDIR)/$(ALGO_O) $(BUILDIR)/queue.o $(BUILDIR)/test_partition.o -o $(PARTITION) $(LDFLAGS)

clean:
	-rm -f $(BINS) $(PARTITION)
