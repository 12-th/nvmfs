#ifndef AVAIL_BLOCK_TABLE_H
#define AVAIL_BLOCK_TABLE_H

#include "Config.h"
#include "Types.h"

struct BlockWearTable;

struct BlockList
{
    struct BlockList * prev;
    struct BlockList * next;
};

struct AvailBlockTable
{
    struct BlockList * blocks;
    UINT64 blockNum;
    struct BlockList * heads;
    int startIndex;
    int endIndex;
};

//所有可用块皆从NVM中恢复，正常情况下序列化到NVM上，非正常情况下直接从NVM的block weartable里面读取数据
void AvailBlockTableRebuild(struct AvailBlockTable * pTable, struct BlockWearTable * pWearTable, UINT64 blockNum);
void AvailBlockTableUninit(struct AvailBlockTable * pTable);
void AvailBlockTableSwapPrepare(struct AvailBlockTable * pTable, physical_block_t oldBlock,
                                physical_block_t * newBlock);
void AvailBlockTableSwapEnd(struct AvailBlockTable * pTable, physical_block_t oldBlock, UINT64 oldBlockIndex,
                            physical_block_t newBlock, UINT64 newBlockIndex);

physical_block_t AvailBlockTableGetBlock(struct AvailBlockTable * pTable);
void AvailBlockTablePutBlock(struct AvailBlockTable * pTable, physical_block_t block, UINT64 index);
// void AvailBlockTableUpdateBlock(struct AvailBlockTable * pTable, physical_block_t block, UINT64 index);

#endif