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