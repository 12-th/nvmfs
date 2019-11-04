#ifndef MAP_TABLE_H
#define MAP_TABLE_H

#include "Types.h"

struct BlockUnmapTable;
struct PageUnmapTable;

struct PgdEntry
{
    unsigned long value;
};

struct PudEntry
{
    unsigned long value;
};

struct PmdEntry
{
    unsigned long value;
};

struct PtEntry
{
    unsigned long value;
};

struct MapTable
{
    struct PgdEntry * entries;
    UINT64 entryNum;
    UINT64 nvmBaseAddr;
    UINT64 dataStartOffset;
};

enum MapLevel
{
    LEVEL_2M,
    LEVEL_4K
};

void MapTableRebuild(struct MapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable,
                     struct PageUnmapTable * pPageUnmapTable, UINT64 blockNum, UINT64 nvmBaseAddr,
                     nvm_addr_t dataStartOffset);
void MapTableUninit(struct MapTable * pTable);
nvm_addr_t MapAddressQuery(struct MapTable * pTable, UINT64 virt);
void BlockMapModify(struct MapTable * pTable, logical_block_t virtBlock, physical_block_t block);
void PageMapModify(struct MapTable * pTable, logical_page_t virtPage, physical_page_t page);
void MapTableSplitBlockMap(struct MapTable * pTable, logical_block_t virtBlock, physical_block_t block);

#endif