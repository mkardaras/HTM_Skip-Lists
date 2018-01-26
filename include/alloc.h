#ifndef ALLOC_H
#define ALLOC_H

#include <stdio.h>
#include <stdlib.h>

#define XMALLOC(var,N) \
	do { \
		var = malloc(N * sizeof(*(var))); \
		if (!(var)) { \
			fprintf(stderr, "Out of memory: %s:%d\n", __FILE__, __LINE__); \
			exit(1); \
		} \
	} while(0)

#endif /* ALLOC_H */
