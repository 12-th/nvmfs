#ifndef ALIGN_H
#define ALIGN_H

#include "Types.h"

#define BITS_4K 12
#define SIZE_4K (1UL << BITS_4K)

#define BITS_2M 21
#define SIZE_2M (1UL << BITS_2M)

static inline UINT64 alignUp(UINT64 size, UINT32 align)
{
    return (size + (align - 1)) / align * align;
}

static inline UINT64 alignUpBits(UINT64 size, UINT8 bits)
{
    return (size + ((1UL << bits) - 1)) & (~((1UL << bits) - 1));
}

static inline UINT64 alignDown(UINT64 size, UINT32 align)
{
    return size / align * align;
}

static inline UINT64 alignDwonBits(UINT64 size, UINT8 bits)
{
    return size & (~((1UL << bits) - 1));
}

#endif