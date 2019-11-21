#ifndef BLOCK_POOL_H
#define BLOCK_POOL_H

#include "ExtentTree.h"
#include <linux/mutex.h>

struct BlockPool
{
    struct ExtentTree tree;
    logical_block_t * blocksCache;
    UINT64 cachedBlockNum;
    struct mutex lock;
};

void BlockPoolInit(struct BlockPool * pool);
void BlockPoolUninit(struct BlockPool * pool);
UINT64 BlockPoolGet(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray);
void BlockPoolExtentGet(struct BlockPool * pool, UINT64 blockNum, struct ExtentContainer * container);
void BlockPoolPut(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray);
void BlockPoolExtentPut(struct BlockPool * pool, struct ExtentContainer * container);

#endif