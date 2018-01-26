/*
 *   File: queue.h
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: simple queue implementation. (header file)
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>        /* perror */
#include <errno.h>        /* errno */
#include <stdlib.h>       /* malloc, free, exit */

#include "ssalloc.h"
#include "ssmem.h"

typedef intptr_t skey_t;

typedef struct queueNode{
    skey_t key;
    struct queueNode *next;
} q_node_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct queueSet
{
  q_node_t *head,*tail;
} q_set_t;

typedef struct commSet
{
  q_set_t *insert_q,*delete_q,*search_q;
} c_set_t;

q_node_t* newQueueNode(skey_t key);
void enq(skey_t key,q_set_t *set);
skey_t deq(q_set_t *set);

q_set_t* queue_set_new();
void queue_set_delete(q_set_t *set);

c_set_t* newCommSet();
void comm_set_delete(c_set_t *set);

#endif /* QUEUE_H */
