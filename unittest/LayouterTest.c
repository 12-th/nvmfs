#include "Layouter.h"
#include "kutest.h"

TEST(LayouterTest, InitTest)
{
    const UINT64 BLOCK_WEAR_TABLE = 1UL << 21;
    const UINT64 BLOCK_UNMAP_TABLE = BLOCK_WEAR_TABLE + (1UL << 21);
    const UINT64 PAGE_WEAR_TABLE = BLOCK_UNMAP_TABLE + (1UL << 21);
    const UINT64 PAGE_UNMAP_TABLE = PAGE_WEAR_TABLE + (1UL << 30);
    const UINT64 MAP_TABLE = PAGE_UNMAP_TABLE + (294UL << 20);
    const UINT64 SWAP_TABLE = MAP_TABLE + (6UL << 20);
    const UINT64 DATA_START = SWAP_TABLE + (256UL << 20);

    struct Layouter l;
    LayouterInit(&l, 40);
    EXPECT_EQ(l.nvmSizeBits, 40);
    EXPECT_EQ(l.blockWearTable, BLOCK_WEAR_TABLE);
    EXPECT_EQ(l.blockUnmapTable, BLOCK_UNMAP_TABLE);
    EXPECT_EQ(l.pageWearTable, PAGE_WEAR_TABLE);
    EXPECT_EQ(l.pageUnmapTable, PAGE_UNMAP_TABLE);
    EXPECT_EQ(l.mapTable, MAP_TABLE);
    EXPECT_EQ(l.swapTable, SWAP_TABLE);
    EXPECT_EQ(l.dataStart, DATA_START);
}