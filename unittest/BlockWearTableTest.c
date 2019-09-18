#include "BlockWearTable.h"
#include "NVMOperations.h"
#include "UnitTestHelp.h"
#include "kutest.h"


TEST(BlockWearTableTest, wholeTest)
{
    struct BlockWearTable tbl;
    const UINT32 BLOCK_NUM = 1UL << 10;
    const UINT32 RAND_ARR_SIZE = 100;
    int i;
    UINT32 * values;
    UINT32 * indice;

    NVMInit(sizeof(UINT32) * BLOCK_NUM);
    BlockWearTableFormat(&tbl, 0, BLOCK_NUM);
    for (i = 0; i < BLOCK_NUM; ++i)
    {
        UINT32 value = GetBlockWearCount(&tbl, i);
        EXPECT_EQ(value, 0);
    }

    values = GenRandomArray(RAND_ARR_SIZE);
    indice = GenSeqArray(RAND_ARR_SIZE);
    Shuffle(indice, RAND_ARR_SIZE);

    for (i = 0; i < RAND_ARR_SIZE; ++i)
    {
        UpdateBlockWearCount(&tbl, indice[i], values[i]);
    }

    for (i = 0; i < RAND_ARR_SIZE; ++i)
    {
        UINT32 value = GetBlockWearCount(&tbl, indice[i]);
        EXPECT_EQ(value, values[i]);
    }

    kfree(indice);
    kfree(values);
    NVMUninit();
}