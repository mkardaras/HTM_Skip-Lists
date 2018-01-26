ROOT = ../..

include $(ROOT)/common/Makefile.common


#################
# Global settings
#################

CFLAGS += -D_REENTRANT -g
CFLAGS += -DLOCKFREE
CFLAGS += -I$(ROOT)/gc
#CFLAGS += -I$(LIBAO_INC) -I$(ROOT)/include

#LDFLAGS += -L$(LIBAO)/lib -latomic_ops 
LDFLAGS += -lpthread -lrt -lm

LINDENFLAGS = -DCACHE_LINE_SIZE=`getconf LEVEL1_DCACHE_LINESIZE` -DINTEL

SPRAY = $(BINDIR)/spray
BINS = $(BINDIR)/*
PROF = $(ROOT)/src
GC = $(ROOT)/gc

.PHONY:	all clean

all:	spray

measurements.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

skiplist.o:
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/skiplist.o skiplist.c

fraser.o: skiplist.h 
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/fraser.o fraser.c 

linden.o: linden.h
	$(CC) $(CFLAGS) $(LINDENFLAGS) -c -o $(BUILDIR)/linden.o linden.c

linden_common.o: linden_common.h
	$(CC) $(CFLAGS) $(LINDENFLAGS) -c -o $(BUILDIR)/linden_common.o linden_common.c

gc.o: $(GC)/gc.h
	$(CC) $(CFLAGS) $(LINDENFLAGS) -c -o $(BUILDIR)/gc.o $(GC)/gc.c

ptst.o: $(GC)/ptst.h
	$(CC) $(CFLAGS) $(LINDENFLAGS) -c -o $(BUILDIR)/ptst.o $(GC)/ptst.c

intset.o: skiplist.h fraser.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: skiplist.h fraser.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o test.c

pqueue.o: skiplist.h intset.h
	$(CC) $(CFLAGS) -c -o $(BUILDIR)/pqueue.o pqueue.c

spray: measurements.o ssalloc.o skiplist.o fraser.o intset.o test.o pqueue.o linden.o linden_common.o gc.o ptst.o
	$(CC) $(CFLAGS) $(BUILDIR)/pqueue.o $(BUILDIR)/measurements.o $(BUILDIR)/ssalloc.o $(BUILDIR)/skiplist.o $(BUILDIR)/fraser.o $(BUILDIR)/intset.o $(BUILDIR)/test.o $(BUILDIR)/linden.o $(BUILDIR)/linden_common.o $(BUILDIR)/ptst.o $(BUILDIR)/gc.o -o $(SPRAY) $(LDFLAGS)

clean:
	-rm -f $(BINS) $(BUILDIR)/*.o
