/*
 *   File: key_operations.h
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description:
 *    Hardware Transactioanl Memory Skip List implementation. (header file)
 *    Implementing Skip List integer key operations using Intel's Restricted Transactional Memory (RTM) interface .
 */

#ifndef KEY_OPERATIONS_H
#define KEY_OPERATIONS_H

#include "skiplist_TM.h"
#include "htm_intel.h"


// SCANHEIGHT is what height to start spray at; must be >= 0
#define SCANHEIGHT floor_log_2(n)+1
// SCANMAX is scanlength at the top level; must be > 0
#define SCANMAX floor_log_2(n)+1
// SCANINC is the amount to increase scan length at each step; can be any integer
#define SCANINC 0
//SCANSKIP is # of levels to go down at each step; must be > 0
#define SCANSKIP 1

/** Search Operation**/
int search(sl_intset_t *set,skey_t key);

/** No Transactional Memory sequential Update Operations **/
int insert_seq(sl_intset_t *set,sl_node_t *node);
int delete_seq(sl_intset_t *set,skey_t key,void *thread_data);

/** Transactional Memory Update Operations **/
int insert_TM(sl_intset_t *set,skey_t key,sval_t value,void *thread_data);
int delete_TM(sl_intset_t *set,skey_t key,void *thread_data);

/**Spray operation**/
int spray(sl_intset_t *set,void *thread_data);

#endif /* KEY_OPERATIONS_H */
