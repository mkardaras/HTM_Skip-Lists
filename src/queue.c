/*
 *   File: queue.c
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: simple queue implementation. (source file)
 *                no need for synchronization since we can have at most one enqueuer and one dequeuer operating at the same time.
 */

#include "queue.h"

q_node_t* newQueueNode(skey_t key){
  q_node_t* node = (q_node_t *)ssalloc_alloc(1, sizeof(q_node_t));
  node->key=key;
  node->next=NULL;
  return node;
}

void enq(skey_t key,q_set_t *set){
  q_node_t *node = newQueueNode(key);
  set->tail->next=node;
  set->tail=node;
}

skey_t deq(q_set_t *set){
  q_node_t *node;
  if (set->head->next == NULL) return -1;
  node=set->head;
  set->head=set->head->next;
  skey_t result = set->head->key;
  ssfree(node);
  return result;
}

q_set_t* queue_set_new(){

  q_set_t *set;
  if ((set = (q_set_t *)ssalloc_aligned(CACHE_LINE_SIZE, sizeof(q_set_t))) == NULL)
  {
    perror("malloc");
    exit(1);
  }
  q_node_t *sentinel= newQueueNode(-1);
  set->head = sentinel;
  set->tail = sentinel;

  return set;
}

void queue_set_delete(q_set_t *set){
  q_node_t *node=set->head;
  q_node_t *head=set->head;
  while(head->next != NULL){
    head=head->next;
    ssfree(node);
    node=head;
  }
  ssfree(node);
  return;
}

c_set_t* newCommSet(){
  c_set_t *set;
  if ((set = (c_set_t *)ssalloc_aligned(CACHE_LINE_SIZE, sizeof(c_set_t))) == NULL)
  {
    perror("malloc");
    exit(1);
  }
  set->insert_q = queue_set_new();
  set->delete_q = queue_set_new();
  set->search_q = queue_set_new();
  return set;
}

void comm_set_delete(c_set_t *set){
  queue_set_delete(set->insert_q);
  queue_set_delete(set->delete_q);
  queue_set_delete(set->search_q );
  ssfree(set);
  return;
}
