#include "Config.h"
#include "LogArea.h"
#include "NVMOperations.h"
#include "UnitTestHelp.h"
#include "kutest.h"
#include <linux/slab.h>

static void * LogEntryPrepare(UINT8 size, void * data)
{
    return kmalloc(size, GFP_KERNEL);
}

static void LogEntryCleanup(UINT8 size, void * buffer, log_type_t type, void * data)
{
    *(UINT32 *)data += *(UINT32 *)buffer;
}

static void LogEntryCleanupEnd(void * data)
{
    (void)data;
}

TEST(LogAreaTest, wholeTest)
{
    struct LogArea logArea;
    UINT32 * datas;
    const int NUM_DATAS = 1UL << 12;
    int i;
    UINT32 sum = 0;
    UINT32 expectSum = 0;
    struct LogAreaCleanupOps ops = {
        .prepare = LogEntryPrepare, .cleanup = LogEntryCleanup, .cleanupEnd = LogEntryCleanupEnd, .data = &sum};
    const UINT8 LOG_TYPE = 1;

    datas = GenRandomArray(NUM_DATAS);

    NVMInit(LOG_AREA_SIZE + SIZE_2M);
    LogAreaFormat(&logArea, 0, SIZE_2M, LOG_AREA_SIZE);
    LogAreaUninit(&logArea);

    LogAreaInit(&logArea, 0);
    LogAreaCleanupOpsRegister(&ops, LOG_TYPE);
    for (i = 0; i < NUM_DATAS; ++i)
    {
        LogAreaWrite(&logArea, &datas[i], sizeof(UINT32), LOG_TYPE);
    }
    LogAreaClean(&logArea);

    for (i = 0; i < NUM_DATAS; ++i)
    {
        expectSum += datas[i];
    }

    EXPECT_EQ(sum, expectSum);
    LogAreaUninit(&logArea);

    NVMUninit();
}
