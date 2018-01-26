.PHONY:	all

BENCHS = src/noise src/skiplist-herlihy_lb src/skiplist-herlihy_lf src/skiplist-seq src/skiplist-htm src/spraylist src/spraylist-htm
LBENCHS = src/skiplist-herlihy_lb
LFBENCHS = src/skiplist-herlihy_lf src/spraylist
SEQBENCHS = src/skiplist-seq
HTMBENCHS = src/skiplist-htm src/spraylist-htm
NOISE = src/noise

.PHONY:	clean all $(BENCHS) $(LBENCHS) $(NOISE) $(TESTS) $(SEQBENCHS) $(HTMBENCHS)

default: lockfree tas seq htm

all:	lockfree tas seq htm

ppopp: slppopp

mutex:
	$(MAKE) "LOCK=MUTEX" $(LBENCHS)

spin:
	$(MAKE) "LOCK=SPIN" $(LBENCHS)

tas:
	$(MAKE) $(LBENCHS)

ticket:
	$(MAKE) "LOCK=TICKET" $(LBENCHS)

hticket:
	$(MAKE) "LOCK=HTICKET" $(LBENCHS)

clh:
	$(MAKE) "LOCK=CLH" $(LBENCHS)

sequential:
	$(MAKE) "STM=SEQUENTIAL" "SEQ_NO_FREE=1" $(SEQBENCHS)

seqgc:
	$(MAKE) "STM=SEQUENTIAL" $(SEQBENCHS)

seq:	sequential

seqsl:
	$(MAKE) "STM=SEQUENTIAL" "SEQ_NO_FREE=1" src/skiplist-seq

seqslgc:
	$(MAKE) "STM=SEQUENTIAL" "GC=1" src/skiplist-seq

lockfree:
	$(MAKE) "STM=LOCKFREE" $(LFBENCHS)

noise:
	$(MAKE) $(NOISE)

htm:
	$(MAKE) $(HTMBENCHS)


lfsl:
	$(MAKE) "STM=LOCKFREE" src/skiplist

lfsl_herlihy_lf:
	$(MAKE) "STM=LOCKFREE" src/skiplist-herlihy_lf

lbsl_herlihy_lb:
	$(MAKE) src/skiplist-herlihy_lb

sl_htm:
	$(MAKE) src/skiplist-htm

sl:	seqsl lfsl_herlihy_lf lbsl_herlihy_lb sl_htm

slppopp: lbsl_herlihy_lb

spray : 
	$(MAKE) src/spraylist

spray_htm :
	$(MAKE) src/spraylist-htm

pq: spray spray_htm

clean:
	$(MAKE) -C src/noise clean
	$(MAKE) -C src/skiplist-herlihy_lb clean
	$(MAKE) -C src/skiplist-herlihy_lf clean
	$(MAKE) -C src/skiplist-seq clean
	$(MAKE) -C src/skiplist-htm clean
	$(MAKE) -C src/spraylist clean
	$(MAKE) -C src/spraylist-htm clean
	rm -rf build

$(SEQBENCHS):
	$(MAKE) -C $@ $(TARGET)

$(LBENCHS):
	$(MAKE) -C $@ $(TARGET)

$(LFBENCHS):
	$(MAKE) -C $@ $(TARGET)

$(HTMBENCHS):
	$(MAKE) -C $@ $(TARGET)

$(NOISE):
	$(MAKE) -C $@ $(TARGET)

