/*
 *   File: htm_intel.h
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description:
 *     Implementing handler for Hardware Transactional Memory transactions,
 *     handling statistics information.
 */


#ifndef RMT_LE_H
#define RMT_LE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtm.h" /* _xbegin() etc. */
#include "alloc.h" /* XMALLOC() */
#include "ssmem.h"
#include "skiplist_TM.h"
#include "queue.h"

#define FORCED_ABORT -10
#define EXP_THRESHOLD 2
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define EXP_POW(f,c,m) (MIN((c) * (f), (m)))

enum {
	TX_ABORT_CONFLICT,
	TX_ABORT_CAPACITY,
	TX_ABORT_EXPLICIT,
	TX_ABORT_REST,
	TX_ABORT_REASONS_END
};

typedef struct ALIGNED(CACHE_LINE_SIZE) {
	int tid;
	sl_intset_t *set;
	c_set_t *read_q;
	c_set_t *write_q;
	unsigned int seed;
	int reached_end;
	int lost_key;
  int nb_add;
  int nb_added;
  int nb_remove;
  int nb_removed;
  int nb_found;
  int nb_contains;
	int tx_starts,
	    tx_commits,
	    tx_aborts,
	    tx_lacqs,
	    tx_forced_aborts;

	int tx_aborts_per_reason[TX_ABORT_REASONS_END];
} tx_thread_data_t;

static inline void *tx_thread_data_new(int tid,sl_intset_t *set,int seed)
{
	tx_thread_data_t *ret;

	XMALLOC(ret, 1);
	memset(ret, 0, sizeof(*ret));
	ret->tid = tid;
	ret->set = set;
	ret->seed = seed;
  ret->tx_forced_aborts = 0;
  ret->nb_add=0;
  ret->nb_added=0;
  ret->nb_remove=0;
  ret->nb_removed=0;
  ret->nb_contains=0;
  ret->nb_found=0;
  ret->reached_end = 0;
  ret->lost_key=0;
	return ret;
}

static inline void *tx_thread_data_new_part(int tid,sl_intset_t *set,int seed,c_set_t *read_q,c_set_t *write_q)
{
	tx_thread_data_t *ret;

	XMALLOC(ret, 1);
	memset(ret, 0, sizeof(*ret));
	ret->tid = tid;
	ret->set = set;
	ret->read_q = read_q;
	ret->write_q = write_q;
	ret->seed = seed;
  ret->tx_forced_aborts = 0;
  ret->nb_add=0;
  ret->nb_added=0;
  ret->nb_remove=0;
  ret->nb_removed=0;
  ret->nb_contains=0;
  ret->nb_found=0;
  ret->reached_end = 0;
  ret->lost_key=0;
	return ret;
}

static inline int tx_start(int num_retries, void *thread_data,
                            pthread_spinlock_t *fallback_lock)
{
	int status = 0;
	int aborts = num_retries;
	tx_thread_data_t *tdata = thread_data;

	while (1) {
		/* Avoid lemming effect. */
		while (*fallback_lock == 0)
			;

		tdata->tx_starts++;

		status = _xbegin();
		if (_XBEGIN_STARTED == (unsigned)status) {
			if (*fallback_lock == 0)
				_xabort(0xff);
			return num_retries - aborts;
		}

		/* Abort comes here. */
		tdata->tx_aborts++;

		if (status & _XABORT_CAPACITY) {
			tdata->tx_aborts_per_reason[TX_ABORT_CAPACITY]++;
		} else if (status & _XABORT_CONFLICT) {
			tdata->tx_aborts_per_reason[TX_ABORT_CONFLICT]++;
    }else if (_XABORT_CODE(status) == 0xaa) {
			tdata->tx_forced_aborts++;
      return FORCED_ABORT;
		}else if (status & _XABORT_EXPLICIT) {
			tdata->tx_aborts_per_reason[TX_ABORT_EXPLICIT]++;
		} else {
			tdata->tx_aborts_per_reason[TX_ABORT_REST]++;
		}


		if (--aborts <= 0) {
			pthread_spin_lock(fallback_lock);
			return num_retries - aborts;
		}
	}

	/* Unreachable. */
	return -1;
}

static inline int tx_end(void *thread_data, pthread_spinlock_t *fallback_lock)
{
	tx_thread_data_t *tdata = thread_data;

	if (*fallback_lock == 1) {
		_xend();
		tdata->tx_commits++;
		return 0;
	} else {
		pthread_spin_unlock(fallback_lock);
		tdata->tx_lacqs++;
		return 1;
	}
}

static inline void unlock(void *thread_data,pthread_spinlock_t *fallback_lock)
{
	tx_thread_data_t *tdata = thread_data;

	pthread_spin_unlock(fallback_lock);
  tdata->tx_lacqs++;
  return;
}


static inline void tx_thread_compute_txs(void *thread_data,unsigned long *txs,unsigned long long *size)
{
	if (!thread_data)
	return;

	tx_thread_data_t *data = thread_data;
	//if (data->tx_commits + data->tx_lacqs != data->tx_total) printf("different %d %d\n",data->tid,data->tx_total);
	*txs += data->nb_add+data->nb_remove+data->nb_contains;
	*size += data->nb_added-data->nb_removed;
	return ;
}


static inline void tx_thread_data_print(void *thread_data)
{
	int i;

	if (!thread_data)
		return;

	tx_thread_data_t *data = thread_data;

	printf("TXSTATS(HASWELL): %3d %12d %12d %12d (", data->tid,
	       data->tx_starts, data->tx_commits, data->tx_aborts);
  printf(" %9d", data->tx_forced_aborts);
	for (i=0; i < TX_ABORT_REASONS_END; i++)
		printf(" %12d", data->tx_aborts_per_reason[i]);
	printf(" )");
	printf(" %12d\n", data->tx_lacqs);
	/*printf(" %8d", data->reached_end);
	printf(" %12d", data->lost_key);
	printf(" %12d\n", data->del);*/
}

static inline void tx_thread_percentages_print(void *thread_data)
{
  if (!thread_data)
		return;

	tx_thread_data_t *data = thread_data;

  float commits,aborts,forced,conflict,capacity,expl,rest,lacqs,aborts_per_operation;
  lacqs = 100*((float)data->tx_lacqs)/(data->tx_lacqs+data->tx_commits);
  commits = 100*((float)data->tx_commits)/data->tx_starts;
  aborts = 100*((float)data->tx_aborts)/data->tx_starts;
  forced = 100*((float)data->tx_forced_aborts)/data->tx_aborts;
  conflict = 100*((float)data->tx_aborts_per_reason[0])/data->tx_aborts;
  capacity = 100*((float)data->tx_aborts_per_reason[1])/data->tx_aborts;
  expl = 100*((float)data->tx_aborts_per_reason[2])/data->tx_aborts;
  rest = 100*((float)data->tx_aborts_per_reason[3])/data->tx_aborts;
  aborts_per_operation = ((float)data->tx_aborts)/(data->nb_added+data->nb_removed);

  printf("total completed update operations : %d\n",data->nb_added+data->nb_removed);
  printf("aborts per update operation : %f \n", aborts_per_operation);
  printf("%f  %% of total update operations committed with lock\n",lacqs);
  printf("%f %% of total transactions where commits\n",commits);
  printf("%f  %% of total transactions where aborts\n",aborts);
  printf("%f  %% of aborts where forced\n",forced);
  printf("%f %% of aborts where conflict\n",conflict);
  printf("%f  %% of aborts where capacity\n",capacity);
  printf("%f  %% of aborts where explicit\n",expl);
  printf("%f  %% of aborts happened for other reasons\n",rest);
  printf("%f %% of total attempts where (effective) update attempts\n",100*((float)data->nb_added+(float)data->nb_removed)/(data->nb_add+data->nb_remove+data->nb_contains));
  printf("%f  %% of attempted search operations found the key\n",100*((float)data->nb_found)/data->nb_contains);
  printf("%f %% of attempted insertions succeded\n",100*((float)data->nb_added)/data->nb_add);
  printf("%f %% of attempted deletions succeded\n",100*((float)data->nb_removed)/data->nb_remove);
  printf("%f  %% of attempted deletions did not find the key\n",100*((float)data->lost_key)/data->nb_remove);
  printf("%f  %% of attempted deletions failed because search went to rightlimit\n",100*((float)data->reached_end)/data->nb_remove);
}

static inline void tx_thread_data_add(void *d1, void *d2, void *dst)
{
	int i;
	tx_thread_data_t *data1 = d1, *data2 = d2, *dest = dst;

	dest->tx_starts = data1->tx_starts + data2->tx_starts;
	dest->tx_commits = data1->tx_commits + data2->tx_commits;
	dest->tx_aborts = data1->tx_aborts + data2->tx_aborts;
	dest->tx_lacqs = data1->tx_lacqs + data2->tx_lacqs;
	dest->tx_forced_aborts = data1->tx_forced_aborts + data2->tx_forced_aborts;
	dest->reached_end=data1->reached_end + data2->reached_end;
	dest->lost_key=data1->lost_key + data2->lost_key;
  dest->nb_add=data1->nb_add +data2->nb_add;
  dest->nb_added=data1->nb_added +data2->nb_added;
  dest->nb_remove=data1->nb_remove +data2->nb_remove;
  dest->nb_removed=data1->nb_removed+data2->nb_removed;
  dest->nb_contains=data1->nb_contains +data2->nb_contains;
  dest->nb_found=data1->nb_found +data2->nb_found;

	for (i=0; i < TX_ABORT_REASONS_END; i++)
		dest->tx_aborts_per_reason[i] = data1->tx_aborts_per_reason[i] +
		                                data2->tx_aborts_per_reason[i];
}

static inline void tx_forced_aborts_add(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;
  tdata->tx_forced_aborts++;
  return;
}

static inline void incr_reached_end(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->reached_end++;
  return;
}

static inline void inc_add(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_add++;
  return;
}

static inline void inc_added(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_added++;
  return;
}

static inline void inc_remove(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_remove++;
  return;
}

static inline void inc_removed(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_removed++;
  return;
}

static inline void inc_containts(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_contains++;
  return;
}

static inline void inc_found(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->nb_found++;
  return;
}


static inline void incr_lost_key(void *thread_data)
{
	tx_thread_data_t *tdata = thread_data;

  tdata->lost_key++;
  return;
}


static inline int compute_unext(void *thread_data,int update)
{
	tx_thread_data_t *tdata = thread_data;

  int ret =((100 * (tdata->nb_added + tdata->nb_removed)) < (update * (tdata->nb_add + tdata->nb_remove + tdata->nb_contains)));
  return ret;
}



#endif /* RMT_LE_H */
