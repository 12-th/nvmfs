#include "NVMOperations.h"
#include "SwapTable.h"
#include "kutest.h"

TEST(SwapTableTest, FormatTest)
{
    struct SwapTable swapTable;
    const UINT64 NVM_SIZE = 10UL << 21;
    const UINT64 SWAP_BLOCK_NUM = 8;
    const UINT64 SWAP_PAGE_NUM = 8;
    const UINT64 SWAP_PAGES_ADDR = 9UL << 21;
    int i;
    nvm_addr_t addr;

    NVMInit(NVM_SIZE);
    SwapTableFormat(&swapTable, 0, SWAP_BLOCK_NUM, SWAP_PAGE_NUM, 1UL << 21, 9UL << 21);
    for (i = 0; i < SWAP_BLOCK_NUM; ++i)
    {
        addr = GetSwapBlock(&swapTable);
        EXPECT_EQ(addr, (i << BITS_2M) + SIZE_2M);
    }
    for (i = 0; i < SWAP_BLOCK_NUM; ++i)
    {
        addr = GetSwapBlock(&swapTable);
        EXPECT_EQ(addr, (i << BITS_2M) + SIZE_2M);
    }

    for (i = 0; i < SWAP_PAGE_NUM; ++i)
    {
        addr = GetSwapPage(&swapTable);
        EXPECT_EQ(addr, SWAP_PAGES_ADDR + (i << BITS_4K));
    }
    for (i = 0; i < SWAP_PAGE_NUM; ++i)
    {
        addr = GetSwapPage(&swapTable);
        EXPECT_EQ(addr, SWAP_PAGES_ADDR + (i << BITS_4K));
    }

    SwapTableUninit(&swapTable);
    NVMUninit();
}