#include "Layouter.h"
#include "kutest.h"

TEST(LayouterTest, InitTest)
{
    const UINT64 BLOCK_META_DATA_TABLE = 1UL << 21;
    const UINT64 AVAIL_BLOCK_TABLE = BLOCK_META_DATA_TABLE + (1UL << 21);
    const UINT64 BLOCK_WEAR_TABLE = AVAIL_BLOCK_TABLE + (16UL << 20);
    const UINT64 BLOCK_UNMAP_TABLE = BLOCK_WEAR_TABLE + (1UL << 21);
    const UINT64 PAGE_WEAR_TABLE = BLOCK_UNMAP_TABLE + (1UL << 21);
    const UINT64 PAGE_UNMAP_TABLE = PAGE_WEAR_TABLE + (1UL << 30);
    const UINT64 MAP_TABLE = PAGE_UNMAP_TABLE + (294UL << 20);
    const UINT64 SWAP_TABLE = MAP_TABLE + (6UL << 20);
    const UINT64 DATA_START = SWAP_TABLE + (256UL << 20);

    struct Layouter l;
    LayouterInit(&l, 40);
    EXPECT_EQ(l.nvmSizeBits, 40);
    EXPECT_TRUE(nvm_ptr_equal(l.blockMetaDataTable, nvm_addr_from_ul(BLOCK_META_DATA_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.availBlockTable, nvm_addr_from_ul(AVAIL_BLOCK_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.blockWearTable, nvm_addr_from_ul(BLOCK_WEAR_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.blockUnmapTable, nvm_addr_from_ul(BLOCK_UNMAP_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.pageWearTable, nvm_addr_from_ul(PAGE_WEAR_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.pageUnmapTable, nvm_addr_from_ul(PAGE_UNMAP_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.mapTable, nvm_addr_from_ul(MAP_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.swapTable, nvm_addr_from_ul(SWAP_TABLE)));
    EXPECT_TRUE(nvm_ptr_equal(l.dataStart, nvm_addr_from_ul(DATA_START)));
}