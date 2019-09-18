#ifndef UNIT_TEST_HELP_H
#define UNIT_TEST_HELP_H

#include "Types.h"
#include <linux/random.h>
#include <linux/slab.h>


static __attribute__ ((unused)) UINT32 * GenRandomArray(UINT32 size)
{
    UINT32 * arr;
    int i;

    arr = kmalloc(sizeof(UINT32) * size, GFP_KERNEL);
    for (i = 0; i < size; ++i)
    {
        UINT32 value;

        get_random_bytes(&value, sizeof(value));
        arr[i] = value;
    }
    return arr;
}

static __attribute__ ((unused)) UINT32 * GenSeqArray(int size)
{
    UINT32 * arr;
    int i;

    arr = kmalloc(sizeof(UINT32) * size, GFP_KERNEL);
    for (i = 0; i < size; ++i)
    {
        arr[i] = i;
    }
    return arr;
}

static __attribute__ ((unused)) UINT32 * Shuffle(UINT32 * arr, int size)
{
    int i;

    for (i = 0; i < size; ++i)
    {
        UINT32 value;
        UINT32 tmp;

        get_random_bytes(&value, sizeof(value));
        value %= size;

        tmp = arr[value];
        arr[value] = arr[i];
        arr[i] = tmp;
    }
    return arr;
}

#endif