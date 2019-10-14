#include "NVMOperations.h"
#include "SwapTable.h"
#include "kutest.h"

TEST(SwapTableTest, FormatTest)
{
    struct SwapTable swapTable;
    const UINT64 NVM_SIZE = 9UL << 21;
    const UINT64 SWAP_BLOCK_NUM = 8;
    int i;
    nvm_addr_t addr;

    NVMInit(NVM_SIZE);
    SwapTableFormat(&swapTable, SWAP_BLOCK_NUM, 0, 0, SIZE_2M);
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

    SwapTableUninit(&swapTable);
    NVMUninit();
}