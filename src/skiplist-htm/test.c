/*
 *   File: test.c
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: Concurrent accesses to a skip list implementation of an integer set
 */

//#define _GNU_SOURCE
#include <sched.h>

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <signal.h>

#include "utils.h"
#include "ssalloc.h"

#include "default_parameters.h"
#include "skiplist_TM.h"
#include "htm_intel.h"
#include "key_operations.h"

#define XSTR(a) STR(a)
#define STR(a)  #a
#define MAX(a,b) ((a) > (b) ? (a) : (b))

// Global variables
pthread_barrier_t start_barrier;
extern int nthreads;
int running;
int last;

__thread unsigned long* seeds;


struct option long_options[] = {
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"skip-list",                 no_argument      , NULL, 'l'},
    {"visual",                    no_argument      , NULL, 'v'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"update-rate",               required_argument, NULL, 'u'},
    {"max-tx-retries",            required_argument, NULL, 'x'},
    {"max-forced-abort-retries",     required_argument, NULL, 'f'},
    {"coarse-grain-transactions", no_argument      , NULL, 'c'},
    {NULL, 0, NULL, 0}
  };

void *thread_fn(void *arg)
{
  int turn=0,key,ret,last_key=last;
  int unext;
  tx_thread_data_t *tx_data = arg;
  sl_intset_t *set = tx_data->set;


  //initialize ssalloc
  ssalloc_init();
  seeds = seed_rand();

  //set affinity
  set_cpu(tx_data->tid);


  /* Is the first op an update? */
  unext = (rand_range_re(&(tx_data->seed),100) - 1 < updateRate);
  int i=0;

	pthread_barrier_wait(&start_barrier);

  while(running){
    if (unext){ // update
      if (turn==0){ //insert
        key=rand_range_re(&(tx_data->seed), max_value);
        if (visual) printf("tid[%d] inserting %d\n",tx_data->tid,key);
        if (coarse_grain_transactions){
          int nodeHeight = get_rand_level();    // find the height of the new node
          sl_node_t *node=createNewNode(key,key,nodeHeight);  // create a new node
          tx_start(max_tx_retries, tx_data, &(set->global_lock));
          ret=insert_seq(set,node);
          tx_end(tx_data, &(set->global_lock));
        }
        else ret=insert_TM(set,key,key,tx_data);
        if (ret) {
          turn=1;
          inc_added(tx_data);
          last_key= key;
        }
        inc_add(tx_data);
      }
      else{ // delete
        if (alternate) key = last_key;
        else key = rand_range_re(&(tx_data->seed), max_value);
        if (visual) printf("tid[%d] deleting %d\n",tx_data->tid,key);
        if (coarse_grain_transactions){
          tx_start(max_tx_retries, tx_data, &(set->global_lock));
          ret=delete_seq(set,key,tx_data);
          tx_end(tx_data, &(set->global_lock));
        }
        else ret=delete_TM(set,key,tx_data);
        if (ret) {
          turn=0;
          inc_removed(tx_data);
        }
        inc_remove(tx_data);
        if (alternate) turn = 0;
      }
      if (visual){ i++; if (i==2) break;}
    }
		else{ // read
      if (alternate) {
        if (updateRate == 0) {
          if (turn == 0) {
            key = last_key;
            turn = 1;
          }
          else {
            key=rand_range_re(&(tx_data->seed), max_value);
            turn = 0;
          }
        }
        else { // updateRate != 0
          if (turn == 0) {
            key=rand_range_re(&(tx_data->seed), max_value);
          }
          else {
            key = last_key;
          }
        }
      }
      else key=rand_range_re(&(tx_data->seed), max_value);
      ret=search(set,key);
      if (ret) inc_found(tx_data);
      inc_containts(tx_data);
		}

    if (effective) { // a failed remove/add is a read-only tx
      unext = compute_unext(tx_data,updateRate);
    } else { // remove/add (even failed) is considered as an update
      unext = (rand_range_re(&(tx_data->seed), 100) - 1 < updateRate);
    }

	}
	return NULL;
}

void catcher(int sig)
{
	printf("CAUGHT SIGNAL %d\n", sig);
}

int main(int argc, char **argv){
  set_cpu(0);
  ssalloc_init();
  seeds = seed_rand();

  int i,j,c,key,seed=DEFAULT_SEED,t_seed;
  struct timeval start, end;
  struct timespec timeout;
  unsigned long long size=0;
  sl_node_t *cur;
  running = 1;

  sl_intset_t *set;
  pthread_t *threads;
  pthread_attr_t attr;

  //> This is an array of per thread TX data.
  void **tx_data;


  // Read user input
  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hcvAd:i:n:r:x:f:s:u:e:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
      case 0:
        break;
      case 'h':
        printf("intset -- STM stress test "
            "(skip list)\n"
            "\n"
            "Usage:\n"
            "  intset [options...]\n"
            "\n"
            "Options:\n"
            "  -h, --help\n"
            "        Print this message\n"
            "  -c, --coarse_grain_transactions\n"
            "        Select for coarse grain transactions implementation. By default fine grain transactions implementation is selected.\n"
            "  -v, --visual\n"
            "        List wil have initial size 100 and each thread will attempt one insertion and one deletion.\n"
            "        The list is printed before and after operations . By default disabled.\n"
            "  -A, --Alternate\n"
            "        Consecutive insert/remove target the same value . By default disabled.\n"
            "  -d, --duration <int>\n"
            "        Test duration in milliseconds (default=" XSTR(DEFAULT_DURATION) ")\n"
            "  -i, --initial-size <int>\n"
            "        Number of elements to insert before test (default=" XSTR(INPUT_LENGTH) ")\n"
            "  -n, --num-threads <int>\n"
            "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
            "  -r, --range <int>\n"
            "        Range of integer values inserted in set (default=" XSTR(MAX_VALUE) ")\n"
            "  -x, --max_tx_retries <int>\n"
            "        Maximum number of transaction aborts/retries before acquiring lock (default=" XSTR(MAX_TX_RETRIES) ")\n"
            "  -f, --max_forced_abort_retries <int>\n"
            "        Maximum number of aborts/retries ,due to inconsistencies, before acquiring lock (default=" XSTR(MAX_FORCED_ABORT_RETRIES) ")\n"
            "  -s, --seed <int>\n"
            "        RNG seed (default=" XSTR(DEFAULT_SEED) ")\n"
            "  -u, --update-rate <int>\n"
            "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE_RATE) ")\n"
            "  -e, --effective <int>\n"
            "        update txs must effectively write (0=trial, 1=effective, default=" XSTR(DEFAULT_EFFECTIVE) ")\n"
            );
        exit(0);
      case 'c':
        ENABLE(coarse_grain_transactions);
        break;
      case 'v':
        ENABLE(visual);
        break;
      case 'A':
        ENABLE(alternate);
        break;
      case 'd':
        duration = atoi(optarg);
        break;
      case 'i':
        input_length = atoi(optarg);
        break;
      case 'n':
        nthreads = atoi(optarg);
        break;
      case 'r':
        max_value = atoi(optarg);
        break;
      case 'x':
        max_tx_retries = atoi(optarg);
        break;
      case 'f':
        max_forced_abort_retries = atol(optarg);
        break;
      case 's':
        seed = atol(optarg);
        break;
      case 'u':
        updateRate = atoi(optarg);
        break;
      case 'e':
        effective = atoi(optarg);
        break;
      case '?':
        printf("Use -h or --help for help\n");
        exit(0);
      default:
        exit(1);
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }

  assert(duration > 0);
  assert(input_length > 0);
  assert(nthreads > 0);
  assert(max_value > 0);
  assert(max_tx_retries >= 0);
  assert(max_forced_abort_retries >= 0);
  assert(seed>0);
  assert(updateRate >= 0 && updateRate <= 100);
  assert(effective == 0 || effective == 1);

  if (visual) {
    duration = DEFAULT_DURATION;
    input_length = 100;
    updateRate = 100;
  }

  // Print parameters
  printf("Set type            : skip list\n");
  if (visual)printf("mode                : visual\n");
  else printf("mode                : non visual\n");
  printf("Duration            : %d (milliseconds)\n", duration);
  printf("Initial size        : %d\n", input_length);
  printf("Nb threads          : %d\n", nthreads);
  printf("Value range         : %d\n", max_value);
  if (alternate) printf("Alternate           : YES\n");
  else printf("Alternate           : NO\n");
  if (coarse_grain_transactions) printf("transactions scheme : coarse grain transactions\n");
  else printf("transactions scheme : fine grain transactions\n");
  printf("Max tx aborts       : %d\n",max_tx_retries);
  printf("Max forced aborts   : %d\n",max_forced_abort_retries);
  printf("seed                : %d\n",seed);
  printf("Update rate         : %d\n", updateRate);
  printf("Efffective          : %d\n", effective);


  // Initialize the structure
  levelmax=MAX(floor_log_2(input_length),floor_log_2(nthreads)+2);
  set = sl_set_new();

  for(i=0;i<input_length;i++){
    key=rand_range_re(NULL, max_value);
    last = key;
    int nodeHeight = get_rand_level();    // find the height of the new node
    sl_node_t *node=createNewNode(key,key,nodeHeight);  // create a new node
    if(!(insert_seq(set,node))) i--;
  }

  size=sl_length(set);
  printf("Initialized skip list with %llu nodes\n", size);


  if (visual){
    cur = set->head;
    for(i=0;i<input_length+1;i++){
      printf("node %" PRIdPTR " ",cur->key);
      for(j=0; j < cur->height;j++) printf(" %" PRIdPTR "  ",cur->next[j]->key);
      printf("\n");
      cur=cur->next[0];
    }
    printf("\n phase2 \n\n");
  }


  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;


	//threads = (pthread_t *)malloc(nthreads* sizeof(pthread_t));
	//if (threads == NULL) {  perror("malloc");  exit(1); }
	XMALLOC(threads,nthreads);
	XMALLOC(tx_data,nthreads);
	pthread_barrier_init(&start_barrier, NULL, nthreads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// initialize seeds for all threads
  if (seed == 0) srand((int)time(0));
  else srand(seed);

	for (i=0; i < nthreads; i++){
    t_seed = rand();
    tx_data[i] = tx_thread_data_new(i,set,t_seed);
    if (pthread_create(&threads[i],  &attr, thread_fn, (void *)(tx_data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
	}

  pthread_attr_destroy(&attr);

  // Catch some signals
  if (signal(SIGHUP, catcher) == SIG_ERR ||
      //signal(SIGINT, catcher) == SIG_ERR ||
      signal(SIGTERM, catcher) == SIG_ERR) {
    perror("signal");
    exit(1);
  }

	//> signal all threads to begin
	pthread_barrier_wait(&start_barrier);

  //STARTING TIMER...
  gettimeofday(&start, NULL);
  nanosleep(&timeout, NULL);
  running = 0;
  gettimeofday(&end, NULL);
   //STOPPING TIMER...

	for (i=0; i < nthreads; i++){
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
	}

	if (visual){
    cur = set->head;
    while(1){
      printf("node % " PRIdPTR " ",cur->key);
      for(j=0; j < cur->height;j++) printf("% " PRIdPTR "  ",cur->next[j]->key);
      printf("\n");
      cur=cur->next[0];
      if (cur->next[0]==NULL) break;
    }
    printf("\n\n");
  }

	/*** PRINTING RESULTS **/
	unsigned long txs=0;
  printf("                   tid     #starts      #commits      #aborts(     FORCED      CONFLICT      CAPACITY      EXPLICIT      REST)        lacqs\n");
  //printf("                   tid      #starts      #commits     #aborts(       TIMESTAMP    CONFLICT      CAPACITY      EXPLICIT      REST)        lacqs       END       LOST_KEY      DELETED\n");
	void *total_tx_data = tx_thread_data_new(-1,set,0);
	for (i=0; i < nthreads; i++) {
    tx_thread_compute_txs(tx_data[i],&txs,&size);
		tx_thread_data_print(tx_data[i]);
		tx_thread_data_add(total_tx_data, tx_data[i], total_tx_data);
	}
	tx_thread_data_print(total_tx_data);
	tx_thread_percentages_print(total_tx_data);
	printf ("duration = %d\n", duration);
  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
  printf ("duration = %d\n", duration);
  printf("#txs         : %lu (%f / s)\n", txs, txs * 1000.0 / duration);
  printf("Finally skip list contains %llu nodes (expected: %llu) \n\n", sl_length(set),size);

  sl_set_delete(set);
  free(threads);
  for(i=0;i<nthreads;i++) free(tx_data[i]);
  free(tx_data);
  /**** END ****/
	return 0;
}
