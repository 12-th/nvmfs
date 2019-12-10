#include "BlockPool.h"
#include "Log.h"
#include "NVMOperations.h"
#include "PagePool.h"
#include "UnitTestHelp.h"
#include "WearLeveler.h"
#include "kutest.h"

#define LogTestVariableDeclare()                                                                                       \
    struct WearLeveler wl;                                                                                             \
    struct NVMAccesser acc;                                                                                            \
    struct BlockPool bpool;                                                                                            \
    struct ExtentContainer container;                                                                                  \
    struct PagePool ppool;                                                                                             \
    struct Log log;

#define LogTestInit()                                                                                                  \
    NVMInit(1UL << 30);                                                                                                \
    WearLevelerFormat(&wl, 30, 0);                                                                                     \
    NVMAccesserInit(&acc, &wl);                                                                                        \
    BlockPoolInit(&bpool);                                                                                             \
    ExtentContainerInit(&container, GFP_KERNEL);                                                                       \
    ExtentContainerAppend(&container, 0, 1UL << 30, GFP_KERNEL);                                                       \
    BlockPoolExtentPut(&bpool, &container);                                                                            \
    ExtentContainerUninit(&container);                                                                                 \
    PagePoolInit(&ppool, &bpool, acc);

#define LogTestUninit()                                                                                                \
    PagePoolUninit(&ppool);                                                                                            \
    BlockPoolUninit(&bpool);                                                                                           \
    NVMAccesserUninit(&acc);                                                                                           \
    WearLevelerUninit(&wl);                                                                                            \
    NVMUninit();

TEST(LogTest, formatTest1)
{
    LogTestVariableDeclare();
    LogTestInit();
    LogFormat(&log, &ppool, 0, &acc);
    LogTestUninit();
}

static void * PrepareForOneNum(UINT32 * size, void * data)
{
    (void)(data);
    static UINT32 num;
    return &num;
}

static void * PrepareForTwoNum(UINT32 * size, void * data)
{
    (void)(data);
    static UINT32 nums[2];
    return nums;
}

static void * PrepareForThreeNum(UINT32 * size, void * data)
{
    (void)(data);
    static UINT32 nums[3];
    return nums;
}

struct LogTestInfo
{
    UINT32 index;
    UINT32 * array;
};

static void CleanupForOneNum(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                             logic_addr_t entryReadEndAddr, void * data)
{
    struct LogTestInfo * info = (struct LogTestInfo *)(data);
    info->array[info->index++] = *((UINT32 *)buffer);
}

static void CleanupForTwoNum(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                             logic_addr_t entryReadEndAddr, void * data)
{
    struct LogTestInfo * info = (struct LogTestInfo *)(data);
    (void)(size);
    (void)(type);
    info->array[info->index++] += ((UINT32 *)buffer)[0];
    info->array[info->index++] += ((UINT32 *)buffer)[1];
}

static void CleanupForThreeNum(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                               logic_addr_t entryReadEndAddr, void * data)
{
    struct LogTestInfo * info = (struct LogTestInfo *)(data);
    info->array[info->index++] += ((UINT32 *)buffer)[0];
    info->array[info->index++] += ((UINT32 *)buffer)[1];
    info->array[info->index++] += ((UINT32 *)buffer)[2];
}

TEST(LogTest, formatTest2)
{
    LogTestVariableDeclare();
    UINT32 * array;
    const UINT64 ARRAY_NUM = 4000;
    UINT64 i = 0;
    struct LogTestInfo info = {.index = 0};

    struct LogCleanupOps ops[] = {{.prepare = PrepareForOneNum, .cleanup = CleanupForOneNum},
                                  {.prepare = PrepareForTwoNum, .cleanup = CleanupForTwoNum},
                                  {.prepare = PrepareForThreeNum, .cleanup = CleanupForThreeNum}};

    LogTestInit();
    info.array = kmalloc(sizeof(UINT32) * ARRAY_NUM, GFP_KERNEL);
    // array = GenRandomArray(ARRAY_NUM);
    array = GenSeqArray(ARRAY_NUM);
    LogFormat(&log, &ppool, 0, &acc);
    while (i < ARRAY_NUM)
    {
        // UINT8 num = GetRandNumber() % 3 + 1;
        UINT8 num = 1;
        if (ARRAY_NUM - i < num)
            num = ARRAY_NUM - i;
        LogWrite(&log, num * sizeof(UINT32), array + i, num - 1, NULL, &ppool, &acc);
        i += num;
    }

    LogForEachEntry(&log, ops, &info, &acc);
    for (i = 0; i < ARRAY_NUM; ++i)
    {
        EXPECT_EQ(array[i], info.array[i]);
    }

    kfree(info.array);
    kfree(array);
    LogTestUninit();
}

TEST(LogTest, formatTest3)
{
    LogTestVariableDeclare();
    UINT32 * array;
    const UINT64 ARRAY_NUM = 4000;
    UINT64 i = 0;
    struct LogTestInfo info = {.index = 0};

    struct LogCleanupOps ops[] = {{.prepare = PrepareForOneNum, .cleanup = CleanupForOneNum},
                                  {.prepare = PrepareForTwoNum, .cleanup = CleanupForTwoNum},
                                  {.prepare = PrepareForThreeNum, .cleanup = CleanupForThreeNum}};

    LogTestInit();
    info.array = kmalloc(sizeof(UINT32) * ARRAY_NUM, GFP_KERNEL);
    array = GenRandomArray(ARRAY_NUM);
    LogFormat(&log, &ppool, 0, &acc);
    while (i < ARRAY_NUM)
    {
        UINT8 num = GetRandNumber() % 3 + 1;
        if (ARRAY_NUM - i < num)
            num = ARRAY_NUM - i;
        LogWrite(&log, num * sizeof(UINT32), array + i, num - 1, NULL, &ppool, &acc);
        i += num;
    }

    LogForEachEntry(&log, ops, &info, &acc);
    for (i = 0; i < ARRAY_NUM; ++i)
    {
        if (array[i] != info.array[i])
            EXPECT_EQ(array[i], info.array[i]);
    }

    kfree(info.array);
    kfree(array);
    LogTestUninit();
}

TEST(LogTest, rebuildTest1)
{
    LogTestVariableDeclare();
    UINT32 * array;
    const UINT64 ARRAY_NUM = 4000;
    UINT64 i = 0;
    struct LogTestInfo info = {.index = 0};
    struct Log log2;
    logic_addr_t firstArea;

    struct LogCleanupOps ops[] = {{.prepare = PrepareForOneNum, .cleanup = CleanupForOneNum},
                                  {.prepare = PrepareForTwoNum, .cleanup = CleanupForTwoNum},
                                  {.prepare = PrepareForThreeNum, .cleanup = CleanupForThreeNum}};

    LogTestInit();
    info.array = kmalloc(sizeof(UINT32) * ARRAY_NUM, GFP_KERNEL);
    array = GenRandomArray(ARRAY_NUM);
    LogFormat(&log, &ppool, 0, &acc);
    while (i < ARRAY_NUM)
    {
        UINT8 num = GetRandNumber() % 3 + 1;
        if (ARRAY_NUM - i < num)
            num = ARRAY_NUM - i;
        LogWrite(&log, num * sizeof(UINT32), array + i, num - 1, NULL, &ppool, &acc);
        i += num;
    }

    firstArea = LogFirstArea(&log);

    LogRebuildBegin(&log2, firstArea, 0);
    LogRebuild(&log2, ops, &info, &acc);
    LogRebuildEnd(&log2, &acc);

    EXPECT_EQ(log.reserveSize, log2.reserveSize);
    EXPECT_EQ(log.writeStart, log2.writeStart);
    EXPECT_EQ(log.cs.firstArea, log2.cs.firstArea);
    EXPECT_EQ(log.cs.restSize, log2.cs.restSize);
    EXPECT_EQ(log.cs.areaNum, log2.cs.areaNum);

    for (i = 0; i < ARRAY_NUM; ++i)
    {
        EXPECT_EQ(array[i], info.array[i]);
    }

    LogUninit(&log);
    LogUninit(&log2);
    kfree(info.array);
    kfree(array);
    LogTestUninit();
}