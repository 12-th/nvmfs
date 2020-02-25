#ifndef MAP_TABLE_H
#define MAP_TABLE_H

#include "Types.h"

struct BlockUnmapTable;
struct PageUnmapTable;

struct PageEntry
{
    union {
        UINT32 value;
        struct
        {
            UINT64 busy : 1;
            UINT64 lock : 22;
            UINT64 relativeIndex : 9;
        };
    };
};

struct PageEntryTable
{
    struct PageEntry entries[512];
};

struct BlockEntry
{
    union {
        struct
        {
            UINT64 fineGrain : 1;
            UINT64 busy : 1;
            UINT64 lock : 22;
            UINT64 physBlockIndex : 39;
        };
        UINT64 value;
    };
    struct PageEntryTable * ptr;
};

struct MapTable
{
    struct BlockEntry * entries;
    UINT64 blockNum;
    UINT64 dataStartOffset;
};

struct MapTableLockHandle
{
    UINT32 value1;
    UINT32 value2;
};

void MapTableFormat(struct MapTable * pTable, UINT64 blockNum, UINT64 dataStartOffset);
void MapTableUninit(struct MapTable * pTable);

nvm_addr_t MapTableQuery(struct MapTable * pTable, logic_addr_t addr);
void MapTableBlockSet(struct MapTable * pTable, logic_addr_t logicAddr, physical_block_t block);
void MapTablePageSet(struct MapTable * pTable, logic_addr_t logicAddr, UINT64 relativeIndex);
int MapTableIsBlock(struct MapTable * pTable, logic_addr_t addr);
int MapTableIsBlockBusy(struct MapTable * pTable, logic_addr_t addr);
int MapTableIsPageBusy(struct MapTable * pTable, logic_addr_t addr);
void MapTableBlockTrim(struct MapTable * pTable, logic_addr_t addr);
void MapTablePageTrim(struct MapTable * pTable, logic_addr_t addr);
void MapTableBlockBusy(struct MapTable * pTable, logic_addr_t addr);
void MapTablePageBusy(struct MapTable * pTable, logic_addr_t addr);
void MapTableMerge(struct MapTable * pTable, logic_addr_t addr);
void MapTableSplit(struct MapTable * pTable, logic_addr_t addr);

void MapTableBlockSwapFast(struct MapTable * pTable, logical_block_t blockA, logical_block_t blockB);
void MapTableBlockSwapSlow(struct MapTable * pTable, logical_block_t blockA, logical_block_t blockB,
                           logical_block_t blockC);
int MapTableLockBlockForSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr);
void MapTableUnlockBlockForSwap(struct MapTable * pTable, logic_addr_t logicAddr);

void MapTablePageSwap(struct MapTable * pTable, logic_addr_t pageA, logic_addr_t pageB);
int MapTableLockBlockForPageSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr);
void MapTableUnlockBlockForPageSwap(struct MapTable * pTable, logic_addr_t logicAddr);
int MapTableLockPageForSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr);
void MapTableUnlockPageForSwap(struct MapTable * pTable, logic_addr_t logicAddr);

struct MapTableLockHandle MapTableLockBlockForRead(struct MapTable * pTable, logic_addr_t addr);
int MapTableUnlockBlockForRead(struct MapTable * pTable, logic_addr_t addr, struct MapTableLockHandle handle);
void MapTableLockBlockForWrite(struct MapTable * pTable, logic_addr_t addr);
void MapTableUnlockBlockForWrite(struct MapTable * pTable, logic_addr_t addr);
struct MapTableLockHandle MapTableLockPageForRead(struct MapTable * pTable, logic_addr_t addr);
int MapTableUnlockPageForRead(struct MapTable * pTable, logic_addr_t addr, struct MapTableLockHandle handle);
void MapTableLockPageForWrite(struct MapTable * pTable, logic_addr_t addr);
void MapTableUnlockPageForWrite(struct MapTable * pTable, logic_addr_t addr);

#endif