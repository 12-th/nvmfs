#include "BlockPool.h"
#include "kutest.h"
#include <linux/slab.h>
#include <linux/sort.h>

TEST(BlockPoolTest, ExtentGetPutTest1)
{
    struct BlockPool pool;
    struct ExtentContainer container;
    const UINT64 GET_SIZE = 512;

    ExtentContainerInit(&container, GFP_KERNEL);
    BlockPoolInit(&pool);
    ExtentContainerAppend(&container, 0, 1UL << 19, GFP_KERNEL);
    BlockPoolExtentPut(&pool, &container);
    ExtentContainerClear(&container);
    BlockPoolExtentGet(&pool, GET_SIZE, &container);

    EXPECT_EQ(container.size, GET_SIZE);
    EXPECT_EQ(container.currentNum, 1);

    ExtentContainerUninit(&container);
    BlockPoolUninit(&pool);
}

TEST(BlockPoolTest, ExtentGetPutTest2)
{
    struct BlockPool pool;
    struct ExtentContainer container;
    const UINT64 GET_SIZE = 512;

    ExtentContainerInit(&container, GFP_KERNEL);
    BlockPoolInit(&pool);
    ExtentContainerAppend(&container, 0, 1UL << 19, GFP_KERNEL);
    BlockPoolExtentPut(&pool, &container);
    ExtentContainerClear(&container);
    BlockPoolExtentGet(&pool, GET_SIZE, &container);
    BlockPoolExtentPut(&pool, &container);
    ExtentContainerClear(&container);
    BlockPoolExtentGet(&pool, GET_SIZE, &container);

    EXPECT_EQ(container.size, GET_SIZE);
    EXPECT_EQ(container.currentNum, 1);

    ExtentContainerUninit(&container);
    BlockPoolUninit(&pool);
}

static int LogicBlockCompare(const void * a, const void * b)
{
    if ((*(logical_block_t *)a) < (*(logical_block_t *)b))
        return -1;
    if ((*(logical_block_t *)a) > (*(logical_block_t *)b))
        return 1;
    return 0;
}

TEST(BlockPoolTest, GetPutTest1)
{
    struct BlockPool pool;
    struct ExtentContainer container;
    const UINT64 GET_SIZE = 512;
    logical_block_t * blockArr;
    UINT64 retNum, i;

    blockArr = kmalloc(sizeof(logical_block_t) * 512, GFP_KERNEL);
    ExtentContainerInit(&container, GFP_KERNEL);
    BlockPoolInit(&pool);
    ExtentContainerAppend(&container, 0, 1UL << 19, GFP_KERNEL);
    BlockPoolExtentPut(&pool, &container);

    retNum = BlockPoolGet(&pool, GET_SIZE, blockArr);
    sort(blockArr, 512, sizeof(logical_block_t), LogicBlockCompare, NULL);
    for (i = 1; i < 512; i++)
    {
        EXPECT_TRUE(blockArr[i - 1] < blockArr[i]);
    }
    EXPECT_EQ(retNum, GET_SIZE);

    ExtentContainerUninit(&container);
    BlockPoolUninit(&pool);
    kfree(blockArr);
}

TEST(BlockPoolTest, GetPutTest2)
{
    struct BlockPool pool;
    struct ExtentContainer container;
    const UINT64 GET_SIZE = 512;
    logical_block_t * blockArr;
    UINT64 retNum, i;

    blockArr = kmalloc(sizeof(logical_block_t) * 512, GFP_KERNEL);
    ExtentContainerInit(&container, GFP_KERNEL);
    BlockPoolInit(&pool);
    ExtentContainerAppend(&container, 0, 511, GFP_KERNEL);
    BlockPoolExtentPut(&pool, &container);

    retNum = BlockPoolGet(&pool, GET_SIZE, blockArr);
    sort(blockArr, 511, sizeof(logical_block_t), LogicBlockCompare, NULL);
    for (i = 1; i < 511; i++)
    {
        EXPECT_TRUE(blockArr[i - 1] < blockArr[i]);
    }
    EXPECT_EQ(retNum, 511);

    ExtentContainerUninit(&container);
    BlockPoolUninit(&pool);
    kfree(blockArr);
}