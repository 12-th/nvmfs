#include "Align.h"
#include "Layouter.h"
#include "SuperBlock.h"
#include "SwapTable.h"
#include "kutest.h"

TEST(LayouterTest, InitTest)
{
    const UINT64 SUPER_BLOCK = 0;
    const UINT64 SWAP_TABLE_METADATA = sizeof(struct NVMSuperBlock);

    const UINT64 BLOCK_WEAR_TABLE = 1UL << 21;
    const UINT64 BLOCK_UNMAP_TABLE = BLOCK_WEAR_TABLE + (1UL << 21);
    const UINT64 PAGE_WEAR_TABLE = BLOCK_UNMAP_TABLE + (1UL << 21);
    const UINT64 PAGE_UNMAP_TABLE = PAGE_WEAR_TABLE + (1UL << 30);
    const UINT64 SWAP_TABLE = PAGE_UNMAP_TABLE + (296UL << 20);
    const UINT64 BLOCK_SWAP_TRANSACTION_LOG_AREA = SWAP_TABLE + (SWAP_TABLE_BLOCK_NUM << BITS_2M);
    const UINT64 MAP_TABLE_SERIALIZE_DATA = BLOCK_SWAP_TRANSACTION_LOG_AREA + BLOCK_SWAP_TRANSACTION_LOG_AREA_SIZE;
    const UINT64 DATA_START = AlignUpBits(MAP_TABLE_SERIALIZE_DATA, BITS_2M);

    struct Layouter l;
    LayouterInit(&l, 40);
    EXPECT_EQ(l.nvmSizeBits, 40);
    EXPECT_EQ(l.superBlock, SUPER_BLOCK);
    EXPECT_EQ(l.swapTableMetadata, SWAP_TABLE_METADATA);

    EXPECT_EQ(l.blockWearTable, BLOCK_WEAR_TABLE);
    EXPECT_EQ(l.blockUnmapTable, BLOCK_UNMAP_TABLE);
    EXPECT_EQ(l.pageWearTable, PAGE_WEAR_TABLE);
    EXPECT_EQ(l.pageUnmapTable, PAGE_UNMAP_TABLE);
    EXPECT_EQ(l.swapTable, SWAP_TABLE);
    EXPECT_EQ(l.mapTableSerializeData, MAP_TABLE_SERIALIZE_DATA);
    EXPECT_EQ(l.dataStart, DATA_START);
}