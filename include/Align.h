#ifndef ALIGN_H
#define ALIGN_H

#include "Types.h"

static inline UINT64 AlignUp(UINT64 size, UINT32 align)
{
    return (size + (align - 1)) / align * align;
}

static inline UINT64 AlignUpBits(UINT64 size, UINT8 bits)
{
    return (size + ((1UL << bits) - 1)) & (~((1UL << bits) - 1));
}

static inline UINT64 AlignDown(UINT64 size, UINT32 align)
{
    return size / align * align;
}

static inline UINT64 AlignDownBits(UINT64 size, UINT8 bits)
{
    return size & (~((1UL << bits) - 1));
}

#endif