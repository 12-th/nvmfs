#ifndef BITMAP_H
#define BITMAP_H

#include "common.h"

struct RadixTreeBitmap
{
    unsigned long bits;
};

#define RADIX_TREE_BITMAP_INVALID_INDEX (sizeof(unsigned long) * 8)
#define RADIX_TREE_BITMAP_MIN_INDEX (0)
#define RADIX_TREE_BITMAP_MAX_INDEX (sizeof(unsigned long) * 8 - 1)

static inline void RadixTreeBitmapSet(struct RadixTreeBitmap * map, unsigned int bitIndex)
{
    WARN(bitIndex > RADIX_TREE_BITMAP_MAX_INDEX, "index is too large");
    map->bits |= (1UL << bitIndex);
}

static inline void RadixTreeBitmapClear(struct RadixTreeBitmap * map, unsigned int bitIndex)
{
    WARN(bitIndex > RADIX_TREE_BITMAP_MAX_INDEX, "index is too large");
    map->bits &= (~(1UL << bitIndex));
}

static inline void InitRadixTreeBitmap(struct RadixTreeBitmap * map)
{
    map->bits = 0;
}

static inline void UninitRadixTreeBitmap(struct RadixTreeBitmap * map)
{
}

static inline unsigned int RadixTreeBitmapFindFirstSetBitIndex(struct RadixTreeBitmap * map, unsigned startIndex)
{
    unsigned long index;
    unsigned long bits;

    if (!(startIndex >= RADIX_TREE_BITMAP_MIN_INDEX && startIndex <= RADIX_TREE_BITMAP_MAX_INDEX))
        return RADIX_TREE_BITMAP_INVALID_INDEX;
    bits = (map->bits & (~((1UL << startIndex) - 1)));
    if (!bits)
        return RADIX_TREE_BITMAP_INVALID_INDEX;

    asm("rep; bsf %1, %0" : "=r"(index) : "rm"(bits));
    return index;
}

static inline unsigned int RadixTreeBitmapFindLastSetBitIndex(struct RadixTreeBitmap * map, unsigned startIndex)
{
    unsigned long index;
    unsigned long bits = map->bits;

    if (!(startIndex >= RADIX_TREE_BITMAP_MIN_INDEX && startIndex <= RADIX_TREE_BITMAP_MAX_INDEX))
        return RADIX_TREE_BITMAP_INVALID_INDEX;
    if (startIndex != RADIX_TREE_BITMAP_MAX_INDEX)
    {
        bits = (map->bits & ((1UL << (startIndex + 1)) - 1));
    }
    if (!bits)
        return RADIX_TREE_BITMAP_INVALID_INDEX;

    asm("bsr %1, %0" : "=r"(index) : "rm"(bits));
    return index;
}

static inline int RadixTreeBitmapQuery(struct RadixTreeBitmap * map, unsigned int index)
{
    return map->bits & (1UL << index);
}

static inline int IsRadixTreeBitmapEmpty(struct RadixTreeBitmap * map)
{
    return !(map->bits);
}

#endif