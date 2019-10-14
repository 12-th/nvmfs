#include "BlockUnmapTable.h"
#include "MapTable.h"
#include "NVMOperations.h"
#include "PageUnmapTable.h"
#include "kutest.h"

TEST(MapTableTest, RebuildTest)
{
    struct MapTable mapTable;
    struct BlockUnmapTable blockUnmapTable;
    struct PageUnmapTable pageUnmapTable;
    const UINT64 NVM_SIZE = 4UL << 21;
    const UINT64 BLOCK_NUM = 100;
    const UINT64 PAGE_NUM = BLOCK_NUM * 512;
    const UINT64 NVM_BASE_ADDR = 0x10000UL;
    const UINT64 NVM_DATA_START_ADDR = 0x10000UL;
    UINT64 i;

    NVMInit(NVM_SIZE);
    BlockUnmapTableFormat(&blockUnmapTable, 0, BLOCK_NUM);
    PageUnmapTableFormat(&pageUnmapTable, &blockUnmapTable, 1UL << 21, PAGE_NUM, 1UL << 21);
    MapTableRebuild(&mapTable, &blockUnmapTable, &pageUnmapTable, BLOCK_NUM, NVM_BASE_ADDR, NVM_DATA_START_ADDR);

    for (i = 0; i < BLOCK_NUM; ++i)
    {
        nvm_addr_t addr;

        addr = MapAddressQuery(&mapTable, i << BITS_2M);
        EXPECT_EQ(addr, (i << BITS_2M) + NVM_DATA_START_ADDR);
    }

    MapTableUninit(&mapTable);
    PageUnmapTableUninit(&pageUnmapTable);
    BlockUnmapTableUninit(&blockUnmapTable);
    NVMUninit();
}

TEST(MapTableTest, ModifyTest)
{
    struct MapTable mapTable;
    struct BlockUnmapTable blockUnmapTable;
    struct PageUnmapTable pageUnmapTable;
    const UINT64 NVM_SIZE = 4UL << 21;
    const UINT64 BLOCK_NUM = 100;
    const UINT64 PAGE_NUM = BLOCK_NUM * 512;
    const UINT64 NVM_BASE_ADDR = 0x10000UL;
    const UINT64 NVM_DATA_START_ADDR = 0x10000UL;
    UINT64 i;

    NVMInit(NVM_SIZE);
    BlockUnmapTableFormat(&blockUnmapTable, 0, BLOCK_NUM);
    PageUnmapTableFormat(&pageUnmapTable, &blockUnmapTable, 1UL << 21, PAGE_NUM, 1UL << 21);
    MapTableRebuild(&mapTable, &blockUnmapTable, &pageUnmapTable, BLOCK_NUM, NVM_BASE_ADDR, NVM_DATA_START_ADDR);

    BlockMapModify(&mapTable, 0, BLOCK_NUM - 1);
    BlockMapModify(&mapTable, BLOCK_NUM - 1, 0);

    for (i = 1; i < BLOCK_NUM - 1; ++i)
    {
        nvm_addr_t addr;

        addr = MapAddressQuery(&mapTable, i << BITS_2M);
        EXPECT_EQ(addr, (i << BITS_2M) + NVM_DATA_START_ADDR);
    }
    {
        nvm_addr_t addr;
        addr = MapAddressQuery(&mapTable, 0);
        EXPECT_EQ(addr, ((BLOCK_NUM - 1) << BITS_2M) + NVM_DATA_START_ADDR);
        addr = MapAddressQuery(&mapTable, (BLOCK_NUM - 1) << BITS_2M);
        EXPECT_EQ(addr, NVM_DATA_START_ADDR);
    }

    MapTableUninit(&mapTable);
    PageUnmapTableUninit(&pageUnmapTable);
    BlockUnmapTableUninit(&blockUnmapTable);
    NVMUninit();
}