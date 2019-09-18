#ifndef RANDOM_H
#define RANDOM_H

#include <stdlib.h>

static inline void get_random_bytes(void * buffer, int size)
{
    int i = 0;
    int value;
    char * ptr;
    char * dst = buffer;

    while (i < size)
    {
        if (i % sizeof(int) == 0)
        {
            ptr = (char *)&value;
            value = rand();
        }
        *dst = *ptr;
        dst++;
        ptr++;
        i++;
    }
}

#endif