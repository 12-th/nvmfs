#ifndef TYPES_H
#define TYPES_H

#include "common.h"

typedef unsigned long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long INT64;
typedef int INT32;
typedef short INT16;
typedef char INT8;

typedef UINT64 nvm_addr_t;
typedef UINT32 physical_block_t;
typedef UINT32 logical_block_t;
typedef UINT64 logic_addr_t;
typedef UINT64 physical_page_t;
typedef UINT64 logical_page_t;

typedef UINT64 nvmfs_ino_t;
typedef UINT8 nvmfs_inode_type;

#define BITS_4K 12
#define SIZE_4K (1UL << BITS_4K)

#define BITS_2M 21
#define SIZE_2M (1UL << BITS_2M)

#define invalid_nvm_addr (-1UL)
#define invalid_block (-1)
#define invalid_page (-1UL)
#define invalid_nvmfs_ino (-1UL)

static inline ALWAYS_INLINE logical_block_t logical_addr_to_block(logic_addr_t addr)
{
    return addr >> BITS_2M;
}

static inline ALWAYS_INLINE logic_addr_t logical_block_to_addr(logical_block_t block)
{
    return block << BITS_2M;
}

static inline ALWAYS_INLINE logic_addr_t logical_page_to_addr(logical_page_t page)
{
    return page << BITS_4K;
}

static inline ALWAYS_INLINE logical_page_t logical_addr_to_page(logic_addr_t addr)
{
    return addr >> BITS_4K;
}
static inline ALWAYS_INLINE logical_page_t logical_block_to_page(logical_block_t block)
{
    return block << (BITS_2M - BITS_4K);
}

static inline ALWAYS_INLINE logical_block_t logical_page_to_block(logical_page_t page)
{
    return page >> (BITS_2M - BITS_4K);
}

#endif