#ifndef PAGE_POOL_H
#define PAGE_POOL_H

#include "NVMAccesser.h"
#include "RadixTree.h"
#include "Types.h"
#include <linux/list.h>
#include <linux/spinlock.h>

struct BlockPool;

struct PageSubPool
{
    UINT64 bitmap[8];
    UINT16 count;
    UINT16 nextIndex;
    physical_block_t block;
    struct list_head list;
};

struct PagePool
{
    UINT64 subPoolNum;
    struct list_head nonFull;
    struct BlockPool * blockPool;
    spinlock_t lock;
    struct NVMAccesser acc;
};

void PagePoolGlobalInit(void);
void PagePoolGlobalUninit(void);

void PagePoolInit(struct PagePool * pool, struct BlockPool * blockPool, struct NVMAccesser acc);
void PagePoolUninit(struct PagePool * pool);
logical_page_t PagePoolAlloc(struct PagePool * pool);
void PagePoolFree(struct PagePool * pool, logical_page_t page);

#endif