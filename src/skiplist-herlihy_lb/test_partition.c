/*
 *   File: test.c
 *   Author: Vincent Gramoli <vincent.gramoli@sydney.edu.au>,
 *  	     Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:
 *   test.c is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "intset.h"
#include "utils.h"
#include "queue.h"

__thread unsigned long* seeds;

ALIGNED(64) uint8_t running[64];

unsigned int global_seed;
#ifdef TLS
__thread unsigned int *rng_seed;
#else /* ! TLS */
pthread_key_t rng_seed_key;
#endif /* ! TLS */
unsigned int levelmax;

typedef ALIGNED(64) struct thread_data
{
  sval_t first;
  c_set_t *read_q;
	c_set_t *write_q;
  long range;
  int update;
  int unit_tx;
  int alternate;
  int effective;
  unsigned long nb_add;
  unsigned long nb_added;
  unsigned long nb_remove;
  unsigned long nb_removed;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_aborts;
  unsigned long nb_aborts_locked_read;
  unsigned long nb_aborts_locked_write;
  unsigned long nb_aborts_validate_read;
  unsigned long nb_aborts_validate_write;
  unsigned long nb_aborts_validate_commit;
  unsigned long nb_aborts_invalid_memory;
  unsigned long max_retries;
  unsigned int seed;
  sl_intset_t *set;
  barrier_t *barrier;
  int id;
  uint8_t padding[16];
} thread_data_t;

void print_skiplist(sl_intset_t *set) {
  sl_node_t *curr;
  int i, j;
  int arr[levelmax];

  for (i=0; i< sizeof arr/sizeof arr[0]; i++) arr[i] = 0;

  curr = set->head;
  do {
    printf("%d", (int) curr->val);
    for (i=0; i<curr->toplevel; i++) {
      printf("-*");
    }
    arr[curr->toplevel-1]++;
    printf("\n");
    curr = curr->next[0];
  } while (curr);
  for (j=0; j<levelmax; j++)
    printf("%d nodes of level %d\n", arr[j], j);
}

static inline skey_t getKey(q_set_t *readSet,q_set_t *writeSet,int low_bound,int upp_bound,unsigned int *seed,int range){
  skey_t key;
  key = deq(readSet);
  if (key == -1){
    key=rand_range_re(seed, range);
    if (!(key >= low_bound && key < upp_bound))
    {
      enq(key,writeSet);
      return(-1);
    }
  }
  return key;
}

void*
test(void *data)
{
  int unext, last = -1;
  skey_t val = 0;

  thread_data_t *d = (thread_data_t *)data;
  int upp_bound= (d->id < 14 || (d->id > 27 && d->id < 42)) ? d->range/2  : d->range ;  // machine dependent specs
  int low_bound= (d->id < 14 || (d->id > 27 && d->id < 42)) ? 0 : d->range/2 ;

  set_cpu(d->id);
  /* Wait on barrier */
  ssalloc_init();
  PF_CORRECTION;

  seeds = seed_rand();

  barrier_cross(d->barrier);

  /* Is the first op an update? */
  unext = (rand_range_re(&d->seed, 100) - 1 < d->update);

  /* while (stop == 0) { */
  while (*running)
    {
      if (unext) { // update

	if (last < 0) { // add
    val = getKey(d->read_q->insert_q,d->write_q->insert_q,low_bound,upp_bound,&(d->seed),d->range);
    if (val == -1) goto point;
	  if (sl_add(d->set, val, val)) {
	    d->nb_added++;
	    last = val;
	  }
	  d->nb_add++;

	} else { // remove

	  if (d->alternate) { // alternate mode (default)
	    if (sl_remove(d->set, last)) {
	      d->nb_removed++;
	    }
	    last = -1;
	  } else {
      val = getKey(d->read_q->delete_q,d->write_q->delete_q,low_bound,upp_bound,&(d->seed),d->range);
      if (val == -1) goto point;
	    /* Remove one random value */
	    if (sl_remove(d->set, val)) {
	      d->nb_removed++;
	      /* Repeat until successful, to avoid size variations */
	      last = -1;
	    }
	  }
	  d->nb_remove++;
	}

      } else { // read

	if (d->alternate) {
	  if (d->update == 0) {
	    if (last < 0) {
	      val = d->first;
	      last = val;
	    } else { // last >= 0
        val = getKey(d->read_q->search_q,d->write_q->search_q,low_bound,upp_bound,&(d->seed),d->range);
        if (val == -1) goto point;
	      last = -1;
	    }
	  } else { // update != 0
	    if (last < 0) {
        val= deq(d->read_q->search_q);
        if (val == -1){
          val = rand_range_re(&d->seed,  d->range);
          if (!(val >= low_bound && val < upp_bound)){
            enq(val,d->write_q->search_q);
            goto point;
          }
        }
	      //last = val;
	    } else {
	      val = last;
	    }
	  }
	}	else {
    val = getKey(d->read_q->search_q,d->write_q->search_q,low_bound,upp_bound,&(d->seed),d->range);
    if (val == -1) goto point;
	}

	PF_START(2);
	if (sl_contains(d->set, val))
	  d->nb_found++;
	PF_STOP(2);
	d->nb_contains++;
      }

      /* Is the next op an update? */
point:      if (d->effective) { // a failed remove/add is a read-only tx
	unext = ((100 * (d->nb_added + d->nb_removed))
		 < (d->update * (d->nb_add + d->nb_remove + d->nb_contains)));
      } else { // remove/add (even failed) is considered as an update
	unext = (rand_range_re(&d->seed, 100) - 1 < d->update);
      }
    }

  PF_PRINT;

  return NULL;
}

void
*xmalloc(size_t size)
{
  void *p = malloc(size);
  if (p == NULL)
    {
      perror("malloc");
      exit(1);
    }
  return p;
}


int
main(int argc, char **argv)
{
#if defined(CLH)
  init_clh_thread(&clh_local_p);
#endif

  set_cpu(0);
  ssalloc_init();
  seeds = seed_rand();

  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"seed",                      required_argument, NULL, 's'},
    {"update-rate",               required_argument, NULL, 'u'},
    {"unit-tx",                   required_argument, NULL, 'x'},
    {"nothing",                   required_argument, NULL, 'l'},
    {NULL, 0, NULL, 0}
  };

  sl_intset_t *set,*set1,*set2;
  int i, c, size,size1,size2;
  sval_t last = 0;
  sval_t val = 0;
  unsigned long reads, effreads, updates, effupds, aborts, aborts_locked_read,
    aborts_locked_write, aborts_validate_read, aborts_validate_write,
    aborts_validate_commit, aborts_invalid_memory, max_retries;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  long range = DEFAULT_RANGE;
  int seed = 0;
  int update = DEFAULT_UPDATE;
  int unit_tx = DEFAULT_ELASTICITY;
  int alternate = DEFAULT_ALTERNATE;
  int effective = DEFAULT_EFFECTIVE;
  sigset_t block_set;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hAf:d:i:n:r:s:u:x:l:"
		    , long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
    case 0:
      /* Flag is automatically set */
      break;
    case 'h':
      printf("ASCYLIB -- stress test "
	     "\n"
	     "\n"
	     "Usage:\n"
	     "  %s [options...]\n"
	     "\n"
	     "Options:\n"
	     "  -h, --help\n"
	     "        Print this message\n"
	     "  -A, --Alternate\n"
	     "        Consecutive insert/remove target the same value\n"
	     "  -f, --effective <int>\n"
	     "        update txs must effectively write (0=trial, 1=effective, default=" XSTR(DEFAULT_EFFECTIVE) ")\n"
	     "  -d, --duration <int>\n"
	     "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
	     "  -i, --initial-size <int>\n"
	     "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
	     "  -n, --num-threads <int>\n"
	     "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
	     "  -r, --range <int>\n"
	     "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
	     "  -s, --seed <int>\n"
	     "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
	     "  -u, --update-rate <int>\n"
	     "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
	     "  -x, --unit-tx (default=1)\n"
	     "        Use unit transactions\n"
	     "        0 = non-protected,\n"
	     "        1 = normal transaction,\n"
	     "        2 = read unit-tx,\n"
	     "        3 = read/add unit-tx,\n"
	     "        4 = read/add/rem unit-tx,\n"
	     "        5 = all recursive unit-tx,\n"
	     "        6 = harris lock-free\n"
	     , argv[0]);
      exit(0);
    case 'A':
      alternate = 1;
      break;
    case 'f':
      effective = atoi(optarg);
      break;
    case 'd':
      duration = atoi(optarg);
      break;
    case 'i':
      initial = atoi(optarg);
      break;
    case 'n':
      nb_threads = atoi(optarg);
      break;
    case 'r':
      range = atol(optarg);
      break;
    case 's':
      seed = atoi(optarg);
      break;
    case 'u':
      update = atoi(optarg);
      break;
    case 'x':
      unit_tx = atoi(optarg);
      break;
    case 'l':
      break;
    case '?':
      printf("Use -h or --help for help\n");
      exit(0);
    default:
      exit(1);
    }
  }

  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0 && (nb_threads % 2 == 0));
  assert(range > 0 && range >= initial);
  assert(update >= 0 && update <= 100);

  printf("Set type     : skip list\n");
  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %ld\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("Lock alg.    : %d\n", unit_tx);
  printf("Alternate    : %d\n", alternate);
  printf("Effective    : %d\n", effective);
  printf("Type sizes   : int=%d/long=%d/ptr=%d/word=%d\n",
	 (int)sizeof(int),
	 (int)sizeof(long),
	 (int)sizeof(void *),
	 (int)sizeof(uintptr_t));

  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  data = (thread_data_t *)xmalloc(nb_threads * sizeof(thread_data_t));
  threads = (pthread_t *)xmalloc(nb_threads * sizeof(pthread_t));

  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);

  levelmax = floor_log_2((unsigned int) initial);
  set1 = sl_set_new();
  set2 = sl_set_new();

  /* stop = 0; */
  *running = 1;


  global_seed = rand();
#ifdef TLS
  rng_seed = &global_seed;
#else /* ! TLS */
  if (pthread_key_create(&rng_seed_key, NULL) != 0) {
    fprintf(stderr, "Error creating thread local\n");
    exit(1);
  }
  pthread_setspecific(rng_seed_key, &global_seed);
#endif /* ! TLS */

  /* Init STM */
  printf("Initializing STM\n");

  /* Populate set */
  printf("Adding %d entries to set\n", initial);
  i = 0;
  while (i < initial) {
    val = rand_range_re(&global_seed, range);
    if (val < (range/2)) {set_cpu(0); set=set1;} // machine dependent specs
    else {set_cpu(14); set=set2;}
    if (sl_add(set, val, 0)) {
      last = val;
      i++;
    }
  }
  set_cpu(0);
  size1 = sl_set_size(set1);
  size2 = sl_set_size(set2);
  size=size1+size2;
  printf("Set size1     : %d\n", size1);
  printf("Set size2     : %d\n", size2);
  printf("Level max    : %d\n", levelmax);

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  int overhead=0;
  printf("Creating threads: ");
  for (i=0; i < nb_threads/2; i++){
      if (i >= 14) overhead = 14;
      c_set_t *a = newCommSet();
      c_set_t *b = newCommSet();
      int z = i+(nb_threads/2);
      printf("%d, ", i);
      data[i].read_q = a;
      data[i].write_q = b;
      data[i].first = last;
      data[i].range = range;
      data[i].update = update;
      data[i].unit_tx = unit_tx;
      data[i].alternate = alternate;
      data[i].effective = effective;
      data[i].nb_add = 0;
      data[i].nb_added = 0;
      data[i].nb_remove = 0;
      data[i].nb_removed = 0;
      data[i].nb_contains = 0;
      data[i].nb_found = 0;
      data[i].nb_aborts = 0;
      data[i].nb_aborts_locked_read = 0;
      data[i].nb_aborts_locked_write = 0;
      data[i].nb_aborts_validate_read = 0;
      data[i].nb_aborts_validate_write = 0;
      data[i].nb_aborts_validate_commit = 0;
      data[i].nb_aborts_invalid_memory = 0;
      data[i].max_retries = 0;
      data[i].seed = rand();
      data[i].set = set1;
      data[i].barrier = &barrier;
      data[i].id = i+overhead;
      if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
	fprintf(stderr, "Error creating thread\n");
	exit(1);
      }

      printf("%d, ", z);
      data[z].read_q = b;
      data[z].write_q = a;
      data[z].first = last;
      data[z].range = range;
      data[z].update = update;
      data[z].unit_tx = unit_tx;
      data[z].alternate = alternate;
      data[z].effective = effective;
      data[z].nb_add = 0;
      data[z].nb_added = 0;
      data[z].nb_remove = 0;
      data[z].nb_removed = 0;
      data[z].nb_contains = 0;
      data[z].nb_found = 0;
      data[z].nb_aborts = 0;
      data[z].nb_aborts_locked_read = 0;
      data[z].nb_aborts_locked_write = 0;
      data[z].nb_aborts_validate_read = 0;
      data[z].nb_aborts_validate_write = 0;
      data[z].nb_aborts_validate_commit = 0;
      data[z].nb_aborts_invalid_memory = 0;
      data[z].max_retries = 0;
      data[z].seed = rand();
      data[z].set = set2 ;
      data[z].barrier = &barrier;
      data[z].id = i+overhead+14;
      if (pthread_create(&threads[z], &attr, test, (void *)(&data[z])) != 0) {
	fprintf(stderr, "Error creating thread\n");
	exit(1);
      }
    }
  printf("\n");
  pthread_attr_destroy(&attr);

  /* Start threads */
  barrier_cross(&barrier);

  printf("STARTING...\n");
  gettimeofday(&start, NULL);
  if (duration > 0) {
    nanosleep(&timeout, NULL);
  } else {
    sigemptyset(&block_set);
    sigsuspend(&block_set);
  }

  /**********/
  /*print_skiplist(set);
    for (i=0; i<256; i++) {
    val = rand_range_re(&global_seed, range);
    printf("inserting %ld\n",val);
    sl_add(set, val, 2);
    print_skiplist(set);
    printf("\n\n\n");
    printf("removing %ld\n",val);
    sl_remove(set, val, 2);
    print_skiplist(set);
    printf("\n\n\n");
    }*/

  *running = 0;

  gettimeofday(&end, NULL);
  printf("STOPPING...\n");

  /* Wait for thread completion */
  for (i = 0; i < nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }

  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) -
    (start.tv_sec * 1000 + start.tv_usec / 1000);
  aborts = 0;
  aborts_locked_read = 0;
  aborts_locked_write = 0;
  aborts_validate_read = 0;
  aborts_validate_write = 0;
  aborts_validate_commit = 0;
  aborts_invalid_memory = 0;
  reads = 0;
  effreads = 0;
  updates = 0;
  effupds = 0;
  max_retries = 0;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    printf("  #add        : %lu\n", data[i].nb_add);
    printf("    #added    : %lu\n", data[i].nb_added);
    printf("  #remove     : %lu\n", data[i].nb_remove);
    printf("    #removed  : %lu\n", data[i].nb_removed);
    printf("  #contains   : %lu\n", data[i].nb_contains);
    printf("  #found      : %lu\n", data[i].nb_found);
    printf("  #aborts     : %lu\n", data[i].nb_aborts);
    printf("    #lock-r   : %lu\n", data[i].nb_aborts_locked_read);
    printf("    #lock-w   : %lu\n", data[i].nb_aborts_locked_write);
    printf("    #val-r    : %lu\n", data[i].nb_aborts_validate_read);
    printf("    #val-w    : %lu\n", data[i].nb_aborts_validate_write);
    printf("    #val-c    : %lu\n", data[i].nb_aborts_validate_commit);
    printf("    #inv-mem  : %lu\n", data[i].nb_aborts_invalid_memory);
    printf("  Max retries : %lu\n", data[i].max_retries);
    aborts += data[i].nb_aborts;
    aborts_locked_read += data[i].nb_aborts_locked_read;
    aborts_locked_write += data[i].nb_aborts_locked_write;
    aborts_validate_read += data[i].nb_aborts_validate_read;
    aborts_validate_write += data[i].nb_aborts_validate_write;
    aborts_validate_commit += data[i].nb_aborts_validate_commit;
    aborts_invalid_memory += data[i].nb_aborts_invalid_memory;
    reads += data[i].nb_contains;
    effreads += data[i].nb_contains +
      (data[i].nb_add - data[i].nb_added) +
      (data[i].nb_remove - data[i].nb_removed);
    updates += (data[i].nb_add + data[i].nb_remove);
    effupds += data[i].nb_removed + data[i].nb_added;
    size += data[i].nb_added - data[i].nb_removed;
    if (max_retries < data[i].max_retries)
      max_retries = data[i].max_retries;
  }
  size_t size_after = sl_set_size(set1) + sl_set_size(set2);
  printf("Set size      : %lu (expected: %d)\n", size_after, size);
  printf("Duration      : %d (ms)\n", duration);
  printf("#txs          : %lu (%f / s)\n", reads + updates,
	 (reads + updates) * 1000.0 / duration);

  printf("#read txs     : ");
  if (effective) {
    printf("%lu (%f / s)\n", effreads, effreads * 1000.0 / duration);
    printf("  #contains   : %lu (%f / s)\n", reads, reads * 1000.0 /
	   duration);
  } else printf("%lu (%f / s)\n", reads, reads * 1000.0 / duration);

  printf("#eff. upd rate: %f \n", 100.0 * effupds / (effupds + effreads));

  printf("#update txs   : ");
  if (effective) {
    printf("%lu (%f / s)\n", effupds, effupds * 1000.0 / duration);
    printf("  #upd trials : %lu (%f / s)\n", updates, updates * 1000.0 /
	   duration);
  } else printf("%lu (%f / s)\n", updates, updates * 1000.0 / duration);

  printf("#aborts       : %lu (%f / s)\n", aborts, aborts * 1000.0 /
	 duration);
  printf("  #lock-r     : %lu (%f / s)\n", aborts_locked_read,
	 aborts_locked_read * 1000.0 / duration);
  printf("  #lock-w     : %lu (%f / s)\n", aborts_locked_write,
	 aborts_locked_write * 1000.0 / duration);
  printf("  #val-r      : %lu (%f / s)\n", aborts_validate_read,
	 aborts_validate_read * 1000.0 / duration);
  printf("  #val-w      : %lu (%f / s)\n", aborts_validate_write,
	 aborts_validate_write * 1000.0 / duration);
  printf("  #val-c      : %lu (%f / s)\n", aborts_validate_commit,
	 aborts_validate_commit * 1000.0 / duration);
  printf("  #inv-mem    : %lu (%f / s)\n", aborts_invalid_memory,
	 aborts_invalid_memory * 1000.0 / duration);
  printf("Max retries   : %lu\n", max_retries);

  /* Delete set */
  sl_set_delete(set1);
  sl_set_delete(set2);
  for(i=0;i<nb_threads;i++) comm_set_delete(data[i].read_q);

#ifndef TLS
  pthread_key_delete(rng_seed_key);
#endif /* ! TLS */

  free(threads);
  free(data);

  return 0;
}