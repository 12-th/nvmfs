#ifndef BLOCK_POOL_H
#define BLOCK_POOL_H

#include "ExtentTree.h"
#include "NVMAccesser.h"
#include <linux/mutex.h>

struct BlockPool
{
    struct ExtentTree tree;
    logical_block_t * blocksCache;
    UINT64 cachedBlockNum;
    UINT64 blockNum;
    struct mutex lock;
    struct NVMAccesser * acc;
};

void BlockPoolInit(struct BlockPool * pool, struct NVMAccesser * acc);
void BlockPoolUninit(struct BlockPool * pool);
UINT64 BlockPoolGet(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray);
void BlockPoolExtentGet(struct BlockPool * pool, UINT64 blockNum, struct ExtentContainer * container);
void BlockPoolPut(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray);
void BlockPoolExtentPut(struct BlockPool * pool, struct ExtentContainer * container);

void BlockPoolRecoveryInit(struct BlockPool * pool, struct NVMAccesser * acc);
void BlockPoolRecoveryNotifyBlockBusy(struct BlockPool * pool, logical_block_t startBlock, UINT64 blockNum);
void BlockPoolRecoveryEnd(struct BlockPool * pool, UINT64 totalBlockNum);

UINT64 BlockPoolQueryInTreeBlockNum(struct BlockPool * pool);

#endif