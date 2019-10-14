#include "BlockUnmapTable.h"
#include "NVMOperations.h"
#include "kutest.h"

static inline int BlockInfoEqual(struct BlockInfo * a, struct BlockInfo * b)
{
    if (a->unmapBlock != b->unmapBlock)
        return 0;
    if (a->busy != b->busy)
        return 0;
    if (a->fineGrain != b->fineGrain)
        return 0;
    return 1;
}

TEST(BlockUnmapTableTest, FormatTest)
{
    const UINT64 BLOCK_NUM = 100;
    const UINT64 NVM_SIZE = 1UL << 21;
    physical_block_t i;

    struct BlockUnmapTable blockUnmapTable;

    NVMInit(NVM_SIZE);
    BlockUnmapTableFormat(&blockUnmapTable, 0, BLOCK_NUM);

    for (i = 0; i < BLOCK_NUM; ++i)
    {
        struct BlockInfo info;
        struct BlockInfo expectedInfo = {.unmapBlock = i, .busy = 0, .fineGrain = 0};
        BlockUnmapTableGet(&blockUnmapTable, i, &info);
        EXPECT_TRUE(BlockInfoEqual(&info, &expectedInfo));
    }

    BlockUnmapTableUninit(&blockUnmapTable);
    NVMUninit();
}

TEST(BlockUnmapTableTest, GetSetTest)
{
    const UINT64 BLOCK_NUM = 100;
    const UINT64 NVM_SIZE = 1UL << 21;

    struct BlockUnmapTable blockUnmapTable;
    struct BlockInfo info1 = {.unmapBlock = 99, .busy = 0, .fineGrain = 0};
    struct BlockInfo info2 = {.unmapBlock = 42, .busy = 1, .fineGrain = 1};
    struct BlockInfo gotInfo1, gotInfo2;

    NVMInit(NVM_SIZE);
    BlockUnmapTableFormat(&blockUnmapTable, 0, BLOCK_NUM);
    BlockUnmapTableSet(&blockUnmapTable, 0, &info1);
    BlockUnmapTableSet(&blockUnmapTable, BLOCK_NUM - 1, &info2);
    BlockUnmapTableGet(&blockUnmapTable, 0, &gotInfo1);
    BlockUnmapTableGet(&blockUnmapTable, BLOCK_NUM - 1, &gotInfo2);
    EXPECT_TRUE(BlockInfoEqual(&info1, &gotInfo1));
    EXPECT_TRUE(BlockInfoEqual(&info2, &gotInfo2));

    BlockUnmapTableUninit(&blockUnmapTable);
    NVMUninit();
}