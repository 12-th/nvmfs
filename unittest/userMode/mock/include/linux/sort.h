#ifndef SORT_H
#define SORT_H

#include <stdlib.h>

#define sort(base, num, size, cmp, swap)    \
        qsort(base, num, size, cmp)

#endif