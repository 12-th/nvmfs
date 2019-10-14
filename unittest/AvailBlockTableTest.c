#include "AvailBlockTable.h"
#include "BlockWearTable.h"
#include "NVMOperations.h"
#include "UnitTestHelp.h"
#include "kutest.h"

static inline void GenRandomBlockWearTable(struct BlockWearTable * pTable, unsigned long blockNum)
{
    UINT32 * arr;
    physical_block_t i;

    arr = GenRandomArray(blockNum);
    for (i = 0; i < blockNum; ++i)
    {
        UpdateBlockWearCount(pTable, i, arr[i]);
    }

    kfree(arr);
}

TEST(AvailBlockTableTest, RebuildTest)
{
    const UINT64 BLOCK_NUM = 100UL;
    const UINT64 NVM_SIZE = 1UL << 21;
    struct BlockWearTable blockWearTable;
    struct AvailBlockTable availBlockTable;
    int i;
    physical_block_t blocksSeq[BLOCK_NUM];

    NVMInit(NVM_SIZE);
    BlockWearTableInit(&blockWearTable, 0, BLOCK_NUM);

    GenRandomBlockWearTable(&blockWearTable, BLOCK_NUM);
    AvailBlockTableRebuild(&availBlockTable, &blockWearTable, BLOCK_NUM);
    for (i = 0; i < BLOCK_NUM; ++i)
    {
        blocksSeq[i] = AvailBlockTableGetBlock(&availBlockTable);
    }

    for (i = 1; i < BLOCK_NUM; ++i)
    {
        EXPECT_TRUE(GetBlockWearCount(&blockWearTable, blocksSeq[i]) >=
                    GetBlockWearCount(&blockWearTable, blocksSeq[i - 1]));
    }

    AvailBlockTableUninit(&availBlockTable);
    BlockWearTableUninit(&blockWearTable);
    NVMUninit();
}

TEST(AvailBlockTableTest, GetPutTest)
{
    const UINT64 BLOCK_NUM = 100UL;
    const UINT64 NVM_SIZE = 1UL << 21;
    struct BlockWearTable blockWearTable;
    struct AvailBlockTable availBlockTable;
    int i;
    physical_block_t blocksSeq[BLOCK_NUM];

    NVMInit(NVM_SIZE);
    BlockWearTableInit(&blockWearTable, 0, BLOCK_NUM);

    BlockWearTableFormat(&blockWearTable, 0, BLOCK_NUM);
    AvailBlockTableRebuild(&availBlockTable, &blockWearTable, BLOCK_NUM);
    for (i = 0; i < BLOCK_NUM; ++i)
    {
        blocksSeq[i] = AvailBlockTableGetBlock(&availBlockTable);
    }

    for (i = 0; i < BLOCK_NUM; ++i)
    {
        AvailBlockTablePutBlock(&availBlockTable, blocksSeq[i], i);
    }

    for (i = 0; i < BLOCK_NUM; ++i)
    {
        EXPECT_EQ(AvailBlockTableGetBlock(&availBlockTable), blocksSeq[i]);
    }

    AvailBlockTableUninit(&availBlockTable);
    BlockWearTableUninit(&blockWearTable);
    NVMUninit();
}