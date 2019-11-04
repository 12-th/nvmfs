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

TEST(WearLevelerTest, FormatTest)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t oldAddr, newAddr;
    int * buffer;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);

    oldAddr = LogicAddressTranslate(&wl, 0);
    NVMWrite(oldAddr, PAGE_SIZE, buffer);
    NVMBlockWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
    newAddr = LogicAddressTranslate(&wl, 0);
    EXPECT_NE(oldAddr, newAddr);
    NVMRead(newAddr, PAGE_SIZE, buffer);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

static void PrepareNVMData(void)
{
    struct WearLeveler wl;
    int * buffer;
    nvm_addr_t addr;
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);
    addr = LogicAddressTranslate(&wl, 0);
    NVMWrite(addr, PAGE_SIZE, buffer);
    NVMBlockWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
    kfree(buffer);
    WearLevelerUninit(&wl);
}

TEST(WearLevelerTest, InitTest)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t newAddr;
    int i;
    int * buffer;

    NVMInit(NVM_SIZE);
    PrepareNVMData();

    WearLevelerInit(&wl);
    newAddr = LogicAddressTranslate(&wl, 0);
    EXPECT_NE(0, newAddr);
    buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    NVMRead(newAddr, PAGE_SIZE, buffer);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, pageSwapTest1)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t oldAddr, newAddr;
    int * buffer;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);

    oldAddr = LogicAddressTranslate(&wl, 0);
    NVMBlockInUse(&wl, oldAddr);
    NVMWrite(oldAddr, PAGE_SIZE, buffer);
    NVMBlockSplit(&wl, 0, oldAddr);
    NVMPageWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
    newAddr = LogicAddressTranslate(&wl, 0);
    EXPECT_NE(oldAddr, newAddr);
    NVMRead(newAddr, PAGE_SIZE, buffer);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, pageSwapTest2)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t oldAddr, newAddr;
    int * buffer;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);

    oldAddr = LogicAddressTranslate(&wl, 0);
    NVMBlockSplit(&wl, 0, oldAddr);
    NVMPageInUse(&wl, oldAddr);
    NVMWrite(oldAddr, PAGE_SIZE, buffer);
    NVMPageWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
    newAddr = LogicAddressTranslate(&wl, 0);
    EXPECT_NE(oldAddr, newAddr);
    NVMRead(newAddr, PAGE_SIZE, buffer);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, pageSwapTest3)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t oldAddr, newAddr;
    int * buffer;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer = PrepareBuffer(0xcccccccc);

    oldAddr = LogicAddressTranslate(&wl, 0);
    NVMBlockSplit(&wl, 0, oldAddr);
    NVMPageInUse(&wl, oldAddr);
    NVMWrite(oldAddr, PAGE_SIZE, buffer);

    for (i = 0; i < 1024; ++i)
    {
        NVMPageWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
    }

    newAddr = LogicAddressTranslate(&wl, 0);
    EXPECT_NE(oldAddr, newAddr);
    NVMRead(newAddr, PAGE_SIZE, buffer);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer[i], 0xcccccccc);
    }

    kfree(buffer);
    WearLevelerUninit(&wl);
    NVMUninit();
}

TEST(WearLevelerTest, pageSwapTest4)
{
    struct WearLeveler wl;
    const UINT64 NVM_SIZE = 1UL << 30;
    nvm_addr_t oldAddr, newAddr;
    int *buffer1, *buffer2;
    int i;

    NVMInit(NVM_SIZE);
    WearLevelerFormat(&wl, 30, 0);
    buffer1 = PrepareBuffer(0xcccccccc);
    buffer2 = PrepareBuffer(0xdddddddd);

    oldAddr = LogicAddressTranslate(&wl, 0);
    NVMBlockSplit(&wl, 0, oldAddr);
    NVMPageInUse(&wl, oldAddr);
    NVMWrite(oldAddr, PAGE_SIZE, buffer1);

    oldAddr = LogicAddressTranslate(&wl, PAGE_SIZE);
    NVMPageInUse(&wl, oldAddr);
    NVMWrite(oldAddr, PAGE_SIZE, buffer2);

    for (i = 0; i < 1024; ++i)
    {
        NVMPageWearCountIncrease(&wl, 0, STEP_WEAR_COUNT + 1);
        NVMPageWearCountIncrease(&wl, PAGE_SIZE, STEP_WEAR_COUNT + 1);
    }

    newAddr = LogicAddressTranslate(&wl, PAGE_SIZE);
    NVMRead(newAddr, PAGE_SIZE, buffer2);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer2[i], 0xdddddddd);
    }

    newAddr = LogicAddressTranslate(&wl, 0);
    NVMRead(newAddr, PAGE_SIZE, buffer1);
    for (i = 0; i < (PAGE_SIZE / sizeof(int)); ++i)
    {
        EXPECT_EQ(buffer1[i], 0xcccccccc);
    }

    kfree(buffer1);
    kfree(buffer2);
    WearLevelerUninit(&wl);
    NVMUninit();
}