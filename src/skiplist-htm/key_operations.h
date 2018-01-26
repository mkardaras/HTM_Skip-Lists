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

/** Search Operation**/
int search(sl_intset_t *set,skey_t key);

/** No Transactional Memory sequential Update Operations **/
int insert_seq(sl_intset_t *set,sl_node_t *node);
int delete_seq(sl_intset_t *set,skey_t key,void *thread_data);

/** Transactional Memory Update Operations **/
int insert_TM(sl_intset_t *set,skey_t key,sval_t value,void *thread_data);
int delete_TM(sl_intset_t *set,skey_t key,void *thread_data);


#endif /* KEY_OPERATIONS_H */
