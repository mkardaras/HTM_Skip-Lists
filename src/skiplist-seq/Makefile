ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/sq-sl
PARTITION = $(BINDIR)/sq-sl_part
PROF = $(ROOT)/src

.PHONY:	all clean

all:	main main_partition

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

queue.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/queue.o $(PROF)/queue.c

skiplist.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist.o skiplist.c

seq.o: skiplist.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/seq.o seq.c

test.o: skiplist.h seq.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)

test_partition.o: skiplist.h seq.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test_partition.o test_partition.c

main: measurements.o ssalloc.o skiplist.o seq.o test.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist.o $(BUILDIR)/seq.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

main_partition: measurements.o ssalloc.o skiplist.o seq.o queue.o test_partition.o 
	$(CC) $(CFLAGS) $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist.o $(BUILDIR)/seq.o $(BUILDIR)/queue.o $(BUILDIR)/test_partition.o -o $(PARTITION) $(LDFLAGS)

clean:
	-rm -f $(BINS) $(PARTITION)
