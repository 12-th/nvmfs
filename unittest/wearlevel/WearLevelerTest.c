#include "NVMOperations.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>

void * PrepareBuffer(int value)
{
    void * ptr;

    ptr = kmalloc(PAGE_SIZE, GFP_KERNEL);
    memset(ptr, value, PAGE_SIZE);
    return ptr;
}

TEST(WearLevelerTest, ReadWriteTest1)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    int * buffer;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);

    WearLevelerWrite(&wl, 0, buffer, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerRead(&wl, 0, buffer, PAGE_SIZE);

    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, ReadWriteTest2)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    const UINT64 BLOCK1_ADDR = 0;
    const UINT64 BLOCK2_ADDR = 1UL << 21;
    int *buffer1, *buffer2;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer1 = PrepareBuffer(0xcccccccc);
    buffer2 = PrepareBuffer(0xdddddddd);

    WearLevelerWrite(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerWrite(&wl, BLOCK2_ADDR, buffer2, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerRead(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer1[i], 0xcccccccc);
    }
    WearLevelerRead(&wl, BLOCK2_ADDR, buffer2, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer2[i], 0xdddddddd);
    }

    kfree(buffer2);
    kfree(buffer1);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, ReadWriteTest3)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    const UINT64 BLOCK1_ADDR = 0;
    int *buffer1, *buffer2;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer1 = PrepareBuffer(0xcccccccc);
    buffer2 = PrepareBuffer(0xdddddddd);

    WearLevelerWrite(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerWrite(&wl, BLOCK1_ADDR, buffer2, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerRead(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer1[i], 0xdddddddd);
    }

    kfree(buffer2);
    kfree(buffer1);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, ReadWriteTest4)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    const UINT64 BLOCK1_ADDR = 0;
    const UINT64 BLOCK2_ADDR = 1UL << 21;
    int *buffer1, *buffer2;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer1 = PrepareBuffer(0xcccccccc);
    buffer2 = PrepareBuffer(0xdddddddd);

    WearLevelerWrite(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerWrite(&wl, BLOCK2_ADDR, buffer1, PAGE_SIZE, STEP_WEAR_COUNT - 3);
    WearLevelerWrite(&wl, BLOCK1_ADDR, buffer2, PAGE_SIZE, STEP_WEAR_COUNT);
    WearLevelerRead(&wl, BLOCK1_ADDR, buffer1, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer1[i], 0xdddddddd);
    }
    WearLevelerRead(&wl, BLOCK2_ADDR, buffer2, PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE / sizeof(int); ++i)
    {
        EXPECT_EQ(buffer2[i], 0xcccccccc);
    }

    kfree(buffer2);
    kfree(buffer1);
    WearLevelerUninit(&wl);
    NVMUninit();
}
