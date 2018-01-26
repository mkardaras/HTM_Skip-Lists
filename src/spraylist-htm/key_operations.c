/*
 *   File: key_operations.c
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description:
 *    Hardware Transactioanl Memory Skip List implementation. (source file)
 *    Implementing Skip List integer key operations using Intel's Restricted Transactional Memory (RTM) interface .
 */

#include "key_operations.h"

static int _MarsagliaXOR(int seed) {
//   const int a =      16807;
  const int a =      123456789;
  const int m =     2147483647;
  const int q =      521288629;  /* m div a */
  const int r =       88675123;  /* m mod a */
  int hi   = seed / q;
  int lo   = seed % q;
  int test = a * lo - r * hi;
  if (test > 0)
    seed = test;
  else
    seed = test + m;

  return seed;
}

 /**
 * Search for a node
 **/
 int search(sl_intset_t *set,skey_t key){
  int h ;
  sl_node_t *curr;
  sl_node_t *pred;
  pred=set->head;
  for ( h = levelmax-1; h >= 0; h--)
  {
    curr = pred->next[h];
    while (key > curr->key)
    {
      pred=curr;
      curr = pred->next[h];
    }
    if (unlikely(key == curr->key)) break;
  }
  if (key == curr->key)
  {
    while(curr->state == INITIAL);
    if (curr->state == DELETED) return 0;
    return 1;
  }
  else return 0;
 }


/**
 * Insert a node
 * (sequential algorithm)
 **/
int insert_seq(sl_intset_t *set,sl_node_t *node)
{
  int h , cmpKey;
  sl_node_t *updateArr[levelmax];
  sl_node_t *curr = set->head;
  int key = node->key;
  int nodeHeight= node->height;

  // find where we should put the new node
  for ( h = levelmax-1; h >= 0; h--)
  {
      cmpKey = curr->next[h]->key;
      while (cmpKey < key )
      {
        curr = curr->next[h];
        cmpKey = (curr->next[h])->key;
      }
      updateArr[h] = curr;
  }


  // TODO : Update the value field of an existing node
  if (curr->next[0]->key == key) return 0;

  for(h = 0; h < nodeHeight; h++ )
  {
    node->next[h] = updateArr[h]->next[h];
    updateArr[h]->next[h] = node;
  }
  node->state=INSERTED;

  MEM_BARRIER;
  return 1;
}

/**
 * Remove a node with a certain key
 * (sequential algorithm)
 **/
int delete_seq(sl_intset_t *set,skey_t key,void *thread_data)
{
  tx_thread_data_t *tx_data = thread_data;
  int h , cmpKey,nodeHeight;
  sl_node_t *cur=set->head;
  sl_node_t *updateArr[levelmax];

  // find where the node is
  for ( h = levelmax-1; h >= 0; h--)
  {
    cmpKey = cur->next[h]->key;
    while (cmpKey < key )
    {
      cur = cur->next[h];
      cmpKey = (cur->next[h])->key;
    }
    updateArr[h] = cur;
  }

  cur = cur->next[0];
  if (cur->key == key)
  {
    nodeHeight=cur->height;
    //update fields
    cur->state = DELETED;
    for(h = 0; h < nodeHeight; h++) {
      updateArr[h]->next[h] = cur->next[h];
    }
    return 1;
  }
  else {
    incr_lost_key(tx_data);
    return 0;
  }
}


/**
 * Insert a node
 * (Transactional algorithm)
 **/
int insert_TM(sl_intset_t *set,skey_t key,sval_t value,void *thread_data)
{
  tx_thread_data_t *tx_data = thread_data;
  int h ,ret ;
  int status;
  int nodeHeight;
  sl_node_t *node;

  sl_node_t *preds[levelmax];
  sl_node_t *succs[levelmax];
  sl_node_t *curr;
  sl_node_t *pred;
  int retry=-1;

start_from_scratch_i:
  retry++;
  if (retry >= max_forced_abort_retries) {
    pthread_spin_lock(&(set->global_lock));
    nodeHeight = get_rand_level();    // find the height of the new node
    node=createNewNode(key,value,nodeHeight);  // create a new node
    ret=insert_seq(set,node);
    unlock(tx_data,&(set->global_lock));
    return ret;
  }

  pred=set->head;
  for ( h = levelmax-1; h >= 0; h--)
  {
    curr = pred->next[h];
    while (key > curr->key)
    {
      pred=curr;
      curr = pred->next[h];
    }
    preds[h] = pred;
    succs[h] = curr;
  }
  if (key == curr->key)
  {
    while(curr->state == INITIAL);
    if (curr->state == DELETED) goto start_from_scratch_i;
    return 0; //if node exists , we return
  }


  nodeHeight = get_rand_level();    // find the height of the new node
  node=createNewNode(key,value,nodeHeight);  // create a new node

  status = tx_start(max_tx_retries, tx_data, &(set->global_lock));
  if (status == FORCED_ABORT){
    goto start_from_scratch_i;
  }

  //check consistency
  for(h = 0; h < nodeHeight; h++ )
  {
    if (preds[h]->next[h] != succs[h] || preds[h]->state == DELETED || succs[h]->state == DELETED){
      //force an abort
      deleteNode(node);
      if (_xtest())
        _xabort(0xaa);
      else {
        pthread_spin_unlock(&(set->global_lock));
        tx_forced_aborts_add(tx_data);
        goto start_from_scratch_i;
      }
    }
  }

  //update fields
  for(h = 0; h < nodeHeight; h++ )
  {
    node->next[h] = succs[h];
  }

  for(h = 0; h < nodeHeight; h++ )
  {
    preds[h]->next[h] = node;
  }

  node->state = INSERTED;
  //commit
  tx_end(tx_data, &(set->global_lock));
  MEM_BARRIER;
  return 1;
}


/**
 * Remove a node with a certain key
 * (Transactional algorithm)
 **/
int delete_TM(sl_intset_t *set,skey_t key,void *thread_data)
{
  tx_thread_data_t *tx_data = thread_data;
  int h , nodeHeight;
  int status;
  int ret;
  sl_node_t *preds[levelmax];
  sl_node_t *curr;
  sl_node_t *pred;

  int retry=-1;

 //After 10 forced aborts we acquire the lock
start_from_scratch_d:
  retry++;
  if (retry >= max_forced_abort_retries) {
    pthread_spin_lock(&(set->global_lock));
    ret=delete_seq(set,key,tx_data);
    unlock(tx_data,&(set->global_lock));
    return ret;
  }

  // find where the node is
  pred=set->head;
  for ( h = levelmax-1; h >= 0; h--)
  {
    curr = pred->next[h];
    while (key > curr->key)
    {
      pred=curr;
      curr = pred->next[h];
    }
    preds[h] = pred;
  }

  //begin transaction
  status=tx_start(max_tx_retries, tx_data, &(set->global_lock));
  if (status == FORCED_ABORT){
    goto start_from_scratch_d;
  }
  else
  {
    if (curr->key == key)
    {
      nodeHeight=curr->height;
      //check consistency
      for(h = 0; h < nodeHeight; h++) {
        if (preds[h]->next[h] != curr || preds[h]->state == DELETED ){
          //force an abort
          if (_xtest())
            _xabort(0xaa);
          else {
            pthread_spin_unlock(&(set->global_lock));
            tx_forced_aborts_add(tx_data);
            goto start_from_scratch_d;
          }
        }
      }

      //update fields
      curr->state = DELETED;
      for(h = 0; h < nodeHeight; h++) {
        preds[h]->next[h] = curr->next[h];
      }

      //commit
      tx_end(tx_data, &(set->global_lock));
      return 1;
    }
    else {
      tx_end(tx_data, &(set->global_lock));
      incr_lost_key(tx_data);
      return 0;
    }
  }
}

/**
 * Spray List Delete Min function
 **/
int spray(sl_intset_t *set,void *thread_data)
{
  tx_thread_data_t *tx_data = thread_data;
  unsigned int n = tx_data->nb_threads;
  sl_node_t *cur;
  int scanlen;
  int height = SCANHEIGHT;
  int scanmax = SCANMAX;
  int scan_inc = SCANINC;
  cur=set->head;

  unsigned int *seed = &(tx_data->seed2);

//#ifndef DISTRIBUTION_EXPERIMENT
// *seed = _MarsagliaXOR(*seed);
//  if (n == 1 || *seed % n/*/floor_log_2(n)*/ == 0) { // n == 1 is equivalent to Lotan-Shavit delete_min
//    d->nb_clean++;
//    return lotan_shavit_delete_min_key(set, key, val, d);
//  }
//#endif

  int h = height;
  int dummy = 0;
  while(1) {
    *seed = _MarsagliaXOR(*seed);
    scanlen = *seed % (scanmax+1);

    while (dummy < n*floor_log_2(n)/2 && scanlen > 0) {
      dummy += (1 << h);
      scanlen--;
    }

    while (scanlen > 0 && cur->next[h]) { // Step right //here: cur->next[0], or cur->next[i]??
      cur = cur->next[h];
      scanlen--;
    }

    if (!cur->next[0]) {
      //end transaction
      incr_reached_end(tx_data);
      return 0; //got to end of list
    }

    scanmax += scan_inc;

    if (h == 0)  break;
    if (h <= SCANSKIP) {
      h = 0;
      continue;
    } // need to guarantee bottom level gets scanned
    h -= SCANSKIP;
  }

  //if (cur->key <0) // still in dummy range
  if(cur==set->head) {// still in dummy range
    add_dummy(tx_data);
    return 0;
  }

  while ((cur->state == DELETED || cur->taken == 1) && cur->next[0] ) cur = cur->next[0]; // Find first non-deleted node

  if (cur->key == INT_MAX){ // reached right corner
    incr_reached_end(tx_data);
    return 0;
  }

  cur->taken = 1;
  if (visual) printf("tid[%d] deleting %d\n",tx_data->tid,cur->key);
  if (coarse_grain_transactions) return delete_seq(set,cur->key,tx_data);
  else return delete_TM(set,cur->key,tx_data);
}
