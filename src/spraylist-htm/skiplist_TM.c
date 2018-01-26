/*
 *   File: skiplist_TM.c
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: Skip list implementation of an integer set. (source file)
 */

#include "skiplist_TM.h"


/**
 * this function defines the height of the new node
 **/
int get_rand_level()
{
  int i, level = 1;
  for (i = 0; i < levelmax - 1; i++)
    {
      if ((rand_range(100)-1) < 50)	level++;
      else break;
    }
  /* 1 <= level <= *levelmax */
  return level;
}

/**
 * Create a new node without setting its next fields.
 **/
sl_node_t* createNewNode(skey_t key, sval_t value, int toplevel)
{
  /** allocating memory for new struct **/
  sl_node_t* node;

  size_t ns = sizeof(sl_node_t) + levelmax * sizeof(sl_node_t *);
  size_t ns_rm = ns % 64;
  if (ns_rm)
	{
    ns += 64 - ns_rm;
	}
  node = (sl_node_t *)ssalloc_alloc(1, ns);
  if (node == NULL)
  {
    perror("malloc");
    exit(1);
  }

  /** allocating memory for padding field **/
  /*ns = toplevel*sizeof(unsigned long);
  node->padding = (unsigned long *)ssalloc_alloc(1, ns);
  if (node->padding == NULL){
        perror("error allocating memory for skip list node padding array");
        exit(EXIT_FAILURE);
  }*/

  node->key = key;
  node->value = value;
  node->height = toplevel;
  node->state = INITIAL;
  node->taken = 0;

  //int i;
  //for(i=0;i<toplevel;i++) node->padding[i]=0;
  MEM_BARRIER;
  return node;
}

/**
 * Delete a node
 **/
void deleteNode(sl_node_t *node)
{
  ssfree_alloc(1, (void*) node);
  return;
}

/**
* Create a skiplist set
**/
sl_intset_t* sl_set_new()
{
  sl_intset_t *set;
  sl_node_t *min, *max;
  int i;

  if ((set = (sl_intset_t *)ssalloc_aligned(CACHE_LINE_SIZE, sizeof(sl_intset_t))) == NULL)
  {
    perror("malloc");
    exit(1);
  }

  min = createNewNode(INT_MIN,INT_MIN,levelmax);
  max = createNewNode(INT_MAX,INT_MAX,levelmax);
  for(i=0;i<levelmax;i++) min->next[i]=max;

  set->head = min;

  pthread_spin_init(&(set->global_lock), PTHREAD_PROCESS_SHARED);

  return set;
}

/**
* Delete the set
**/
void sl_set_delete(sl_intset_t *set)
{
  sl_node_t *node, *next;

  node = set->head;
  while (node != NULL)
  {
    next = node->next[0];
    deleteNode(node);
    node = next;
  }
  pthread_spin_destroy(&(set->global_lock));
  ssfree((void*) set);
}


/**
* Return the length of the set
**/
unsigned long long sl_length(sl_intset_t *set)
{
  unsigned long long len = 0;
  sl_node_t *cur;

  for (cur=set->head; cur!=NULL; cur = cur->next[0])
    len++;

  return len-2; // we exclude head and tail
}
