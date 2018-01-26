/*
 *   File: skiplist_TM.h
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: Skip list implementation of an integer set. (header file)
 */

#ifndef SKIPLIST_TM_H
#define SKIPLIST_TM_H

#include <stdio.h>        /* perror */
#include <errno.h>        /* errno */
#include <stdlib.h>       /* malloc, free, exit */
#include <limits.h>       /* INT_MIN */
#include <pthread.h>      /* pthread_spinlock_t */

#include <math.h>
#include "ssalloc.h"
#include "random.h"
#include "ssmem.h"

typedef intptr_t skey_t;
typedef intptr_t sval_t;

extern pthread_spinlock_t global_lock;
extern int levelmax;
extern int visual;
extern int max_tx_retries;
extern int max_forced_abort_retries;


static int floor_log_2(unsigned int n)
{
  int pos = 0;
  if (n >= 1<<16) { n >>= 16; pos += 16; }
  if (n >= 1<< 8) { n >>=  8; pos +=  8; }
  if (n >= 1<< 4) { n >>=  4; pos +=  4; }
  if (n >= 1<< 2) { n >>=  2; pos +=  2; }
  if (n >= 1<< 1) {           pos +=  1; }
  return ((n == 0) ? (-1) : pos);
}

/*#define LOG3(n) floor_log_2(n)*floor_log_2(n)*floor_log_2(n)
#define LOG2(n) floor_log_2(n)*floor_log_2(n)
#define LOGLOG(n) floor_log_2(floor_log_2(n))*/

#define EXPLICIT_ABORT -10

#define INITIAL 0
#define INSERTED 1
#define DELETED 2


/* skip list node definition*/
typedef ALIGNED(CACHE_LINE_SIZE) volatile struct skipListNode{
    skey_t key;
    sval_t value;
    uint32_t height;
    volatile uint8_t state;
    //volatile uint32_t padding[4];
    volatile struct skipListNode *next[1];
} sl_node_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct skipListSet
{
  sl_node_t *head;

  //> A global lock used for the non-tx fallback path if multiple consecutive
  //  transactions fail to commit.
  pthread_spinlock_t global_lock;
} sl_intset_t;

int get_rand_level(); // this function defines the height of the new node

sl_node_t* createNewNode(skey_t key, sval_t value, int toplevel); // Create a new node without setting its next fields.
void deleteNode(sl_node_t *node);

sl_intset_t* sl_set_new();
void sl_set_delete(sl_intset_t *set);
unsigned long long sl_length(sl_intset_t *set); // this function returns the length of the struct

#endif /* SKIPLIST_TM_H */
