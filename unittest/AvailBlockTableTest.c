#include "AvailBlockTable.h"
#include "BlockWearTable.h"
#include "NVMOperations.h"
#include "UnitTestHelp.h"
#include "kutest.h"
#include <linux/sort.h>

static UINT32 * GenBlockWearTable(int startIndex, int endIndex, int blockNum)
{
    UINT32 * arr;
    int i;

    arr = GenRandomArray(blockNum);
    for (i = 0; i < blockNum; ++i)
    {
        arr[i] = ((arr[i] % (endIndex - startIndex)) + startIndex) * STEP_WEAR_COUNT;
    }
    return arr;
}

static void GenAvailBlockTable(struct AvailBlockTable * pTable, UINT32 * arr, UINT64 blockNum)
{
    int i;
    for (i = 0; i < blockNum; ++i)
    {
        int index;
        index = arr[i] / STEP_WEAR_COUNT;
        AvailBlockTablePutBlocks(pTable, i, index);
    }
}

static int BlockCompare(const void * a, const void * b)
{
    return (*(block_t *)a) < (*(block_t *)b);
}

static void AvailBlockTableSort(struct AvailBlockTable * pTable)
{
    struct BlockBuffer * pBuffer;
    int i;
    int limit = pTable->endIndex - pTable->startIndex + 1;

    for (i = 0; i < limit; ++i)
    {
        pBuffer = &pTable->buffers[i];
        if (pBuffer->blocks && pBuffer->size)
        {
            sort(pBuffer->blocks, pBuffer->size, sizeof(block_t), BlockCompare, NULL);
        }
    }
}

static int IsAvailBlockEqual(struct AvailBlockTable * tbl1, struct AvailBlockTable * tbl2)
{
    int limit;
    int i, j;

    if (tbl1->startIndex != tbl2->startIndex)
        return 0;
    if (tbl1->endIndex != tbl2->endIndex)
        return 0;
    limit = tbl1->endIndex - tbl1->startIndex + 1;
    for (i = 0; i < limit; ++i)
    {
        struct BlockBuffer *pb1, *pb2;
        pb1 = &tbl1->buffers[i];
        pb2 = &tbl2->buffers[i];
        if (pb1->size != pb2->size)
            return 0;
        for (j = 0; j < pb1->size; ++j)
        {
            if (pb1->blocks[j] != pb2->blocks[j])
                return 0;
        }
    }
    return 1;
}

TEST(AvailBlockTableTest, SerializeTest)
{
    UINT32 * wearArr;
    struct AvailBlockTable tbl1, tbl2, tbl3;
    struct BlockWearTable bwt;
    const int BLOCK_NUM = 10000;
    const nvm_addr_t WEAR_TABLE = 0;
    const nvm_addr_t AVAIL_BLOCK_TABLE = 1UL << 21;

    NVMInit(10UL << 20);
    wearArr = GenBlockWearTable(0, 100, BLOCK_NUM);
    BlockWearTableInit(&bwt, WEAR_TABLE, BLOCK_NUM);
    GenAvailBlockTable(&tbl1, wearArr, BLOCK_NUM);
    AvailBlockTableSerialize(&tbl1, AVAIL_BLOCK_TABLE);
    AvailBlockTableDeserialize(&tbl2, AVAIL_BLOCK_TABLE);
    NVMWrite(0, BLOCK_NUM * sizeof(UINT32), wearArr);
    AvailBlockTableRebuild(&tbl3, &bwt, BLOCK_NUM);

    AvailBlockTableSort(&tbl1);
    AvailBlockTableSort(&tbl2);
    AvailBlockTableSort(&tbl3);
    EXPECT_TRUE(IsAvailBlockEqual(&tbl1, &tbl2));
    EXPECT_TRUE(IsAvailBlockEqual(&tbl1, &tbl3));

    AvailBlockTableUninit(&tbl1);
    AvailBlockTableUninit(&tbl2);
    AvailBlockTableUninit(&tbl3);
    kfree(wearArr);
    NVMUninit();
}