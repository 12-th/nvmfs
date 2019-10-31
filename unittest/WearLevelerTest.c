#include "NVMOperations.h"
#include "WearLeveler.h"
#include "kutest.h"
#include <linux/slab.h>

void * PrepareBuffer(void)
{
    void * ptr;

    ptr = kmalloc(PAGE_SIZE, GFP_KERNEL);
    memset(ptr, 0xcccccccc, PAGE_SIZE);
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
    buffer = PrepareBuffer();

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
    buffer = PrepareBuffer();
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
    buffer = PrepareBuffer();

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
    buffer = PrepareBuffer();

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

TEST(WearLevelerTest, blockSwapTest)
{
}