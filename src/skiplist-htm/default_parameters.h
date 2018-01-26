/*
 *   File: default_parameters.h
 *   Author: Marios Kardaras <marioskard@hotmail.com>
 *   Description: Initializing default parameters used for htm implementation.
 */

#ifndef DEFAULT_PARAMETERS_H
#define DEFAULT_PARAMETERS_H

// DEFAULT_PARAMETERS

#define DEFAULT_DURATION 1000
#define INPUT_LENGTH 1000000
#define MAX_VALUE 100000000
#define DEFAULT_NB_THREADS 8
#define MAX_TX_RETRIES 30
#define MAX_FORCED_ABORT_RETRIES 15
#define DEFAULT_SEED 60
#define DEFAULT_UPDATE_RATE 100
#define DEFAULT_EFFECTIVE 1
#define ENABLE(x) x=1
#define DISABLE(x) x=0


int duration = DEFAULT_DURATION;
int input_length = INPUT_LENGTH;
int nthreads =  DEFAULT_NB_THREADS;
int max_value = MAX_VALUE;
int levelmax; // maximum possible level.
int max_tx_retries = MAX_TX_RETRIES;
int max_forced_abort_retries = MAX_FORCED_ABORT_RETRIES;
int updateRate = DEFAULT_UPDATE_RATE;
int effective = DEFAULT_EFFECTIVE;
// DO NOT CHANGE THE FOLLOWING. INSTEAD USE CONSOLE ARGUMENTS.
DISABLE(int visual);
DISABLE(int coarse_grain_transactions); // disable for fine-grained transactions solution / enable for coarse-grained transactions solution .
DISABLE(int alternate);

#endif
