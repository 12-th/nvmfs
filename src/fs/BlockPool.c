#include "BlockPool.h"
#include <linux/slab.h>

#define MAX_CACHE_BLOCK_NUM 5

void BlockPoolInit(struct BlockPool * pool, struct NVMAccesser * acc)
{
    ExtentTreeInit(&pool->tree);
    pool->blocksCache = kmalloc(sizeof(logical_block_t) * MAX_CACHE_BLOCK_NUM, GFP_KERNEL);
    pool->cachedBlockNum = 0;
    pool->blockNum = 0;
    mutex_init(&pool->lock);
    pool->acc = acc;
}

void BlockPoolUninit(struct BlockPool * pool)
{
    ExtentTreeUninit(&pool->tree);
    mutex_uninit(&pool->lock);
}

UINT64 BlockPoolGet(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray)
{
    UINT64 gotBlockNum = 0;
    UINT64 batchPullBlockNum = 0;
    struct ExtentContainer container;
    struct Extent * extent;
    UINT64 j;

    ExtentContainerInit(&container, GFP_KERNEL);
    mutex_lock(&pool->lock);
    if (pool->cachedBlockNum >= blockNum)
    {
        memcpy(blockArray, pool->blocksCache + pool->cachedBlockNum - blockNum, blockNum * sizeof(logical_block_t));
        pool->cachedBlockNum -= blockNum;
        mutex_unlock(&pool->lock);
        return blockNum;
    }

    batchPullBlockNum = MAX_CACHE_BLOCK_NUM / 2 + blockNum - pool->cachedBlockNum;
    ExtentTreeGet(&pool->tree, &container, batchPullBlockNum, GFP_KERNEL);
    if (!container.size)
        goto out;
    if (pool->cachedBlockNum)
    {
        memcpy(blockArray, pool->blocksCache, pool->cachedBlockNum * sizeof(logical_block_t));
        gotBlockNum = pool->cachedBlockNum;
        pool->cachedBlockNum = 0;
    }
    for_each_extent_in_container(extent, &container)
    {
        for (j = extent->start; j < extent->end; ++j)
        {
            if (gotBlockNum < blockNum)
            {
                blockArray[gotBlockNum] = j;
                gotBlockNum++;
            }
            else
            {
                pool->blocksCache[pool->cachedBlockNum] = j;
                pool->cachedBlockNum++;
            }
        }
    }
out:
    pool->blockNum -= gotBlockNum;
    mutex_unlock(&pool->lock);
    ExtentContainerUninit(&container);
    return gotBlockNum;
}

void BlockPoolExtentGet(struct BlockPool * pool, UINT64 blockNum, struct ExtentContainer * container)
{
    mutex_lock(&pool->lock);
    ExtentTreeGet(&pool->tree, container, blockNum, GFP_KERNEL);
    pool->blockNum -= container->size;
    mutex_unlock(&pool->lock);
}

static void TrimBlockWithArray(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray)
{
    int i = 0;
    if (pool->acc)
    {
        for (i = 0; i < blockNum; ++i)
        {
            NVMAccesserTrim(pool->acc, logical_block_to_addr(blockArray[i]));
        }
    }
}

static void TrimBlockWithContainer(struct BlockPool * pool, struct ExtentContainer * container)
{
    struct Extent * extent;
    int i;

    if (pool->acc)
    {
        for_each_extent_in_container(extent, container)
        {
            for (i = extent->start; i < extent->end; ++i)
            {
                NVMAccesserTrim(pool->acc, logical_block_to_addr(i));
            }
        }
    }
}

void BlockPoolPut(struct BlockPool * pool, UINT64 blockNum, logical_block_t * blockArray)
{
    UINT64 outToTreeBlockNum;

    TrimBlockWithArray(pool, blockNum, blockArray);

    mutex_lock(&pool->lock);
    pool->blockNum += blockNum;
    if (blockNum + pool->cachedBlockNum <= MAX_CACHE_BLOCK_NUM)
    {
        memcpy(pool->blocksCache + pool->cachedBlockNum, blockArray, blockNum * sizeof(logical_block_t));
        pool->cachedBlockNum += blockNum;
        mutex_unlock(&pool->lock);
        return;
    }

    outToTreeBlockNum = pool->cachedBlockNum + blockNum - MAX_CACHE_BLOCK_NUM / 2;
    if (outToTreeBlockNum > pool->cachedBlockNum)
    {
        int i;
        for (i = 0; i < pool->cachedBlockNum; ++i)
        {
            ExtentTreePut(&pool->tree, pool->blocksCache[i], pool->blocksCache[i] + 1, GFP_KERNEL);
        }
        for (i = 0; i < outToTreeBlockNum - pool->cachedBlockNum; ++i)
        {
            ExtentTreePut(&pool->tree, blockArray[i], blockArray[i] + 1, GFP_KERNEL);
        }
        memcpy(pool->blocksCache, blockArray + i, (blockNum - i) * sizeof(logical_block_t));
        pool->cachedBlockNum = blockNum - i;
    }
    else
    {
        int i;
        for (i = 0; i < outToTreeBlockNum; ++i)
        {
            ExtentTreePut(&pool->tree, pool->blocksCache[i], pool->blocksCache[i] + 1, GFP_KERNEL);
        }
        memcpy(pool->blocksCache, pool->blocksCache + outToTreeBlockNum,
               (pool->cachedBlockNum - outToTreeBlockNum) * sizeof(logical_block_t));
        pool->cachedBlockNum -= outToTreeBlockNum;
        for (i = 0; i < blockNum; ++i)
        {
            pool->blocksCache[pool->cachedBlockNum] = blockArray[i];
            pool->cachedBlockNum++;
        }
    }
    mutex_unlock(&pool->lock);
}

void BlockPoolExtentPut(struct BlockPool * pool, struct ExtentContainer * container)
{
    TrimBlockWithContainer(pool, container);
    mutex_lock(&pool->lock);
    pool->blockNum += container->size;
    ExtentTreeBatchPut(&pool->tree, container, GFP_KERNEL);
    mutex_unlock(&pool->lock);
}

void BlockPoolRecoveryInit(struct BlockPool * pool, struct NVMAccesser * acc)
{
    BlockPoolInit(pool, acc);
}

void BlockPoolRecoveryNotifyBlockBusy(struct BlockPool * pool, logical_block_t startBlock, UINT64 blockNum)
{
    ExtentTreePut(&pool->tree, startBlock, startBlock + blockNum, GFP_KERNEL);
}

void BlockPoolRecoveryEnd(struct BlockPool * pool, UINT64 totalBlockNum)
{
    ExtentTreeReverseHolesAndExtents(&pool->tree, totalBlockNum);
    pool->blockNum = totalBlockNum - pool->blockNum;
}

UINT64 BlockPoolQueryInTreeBlockNum(struct BlockPool * pool)
{
    struct ExtentNode * extent;
    struct rb_node * rbnode;
    UINT64 sum = 0;
    for_each_extent_in_extent_tree(extent, rbnode, &pool->tree)
    {
        sum += extent->extent.end - extent->extent.start;
    }
    return sum;
}