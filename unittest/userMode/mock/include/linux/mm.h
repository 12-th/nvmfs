#ifndef MM_H
#define MM_H

#include <stdlib.h>

#define kvmalloc(size, gfp) malloc(size)

#ifndef kvfree
#define kvfree(ptr) free(ptr)
#endif

#endif