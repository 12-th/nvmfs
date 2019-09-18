#ifndef AVAIL_BLOCK_TABLE_H
#define AVAIL_BLOCK_TABLE_H

#include "Config.h"
#include "Types.h"
#include <linux/spinlock.h>

struct BlockWearTable;

struct BlockBuffer
{
    spinlock_t lock;
    UINT32 size;
    int capacity;
    block_t * blocks;
};

struct AvailBlockTable
{
    int startIndex;
    int endIndex;
    struct BlockBuffer * buffers;
};

//所有可用块皆从NVM中恢复，正常情况下序列化到NVM上，非正常情况下直接从NVM的block weartable里面读取数据
void AvailBlockTableSerialize(struct AvailBlockTable * pTable, nvm_addr_t pAvailBlockTable);
void AvailBlockTableDeserialize(struct AvailBlockTable * pTable, nvm_addr_t pAvailBlockTable);
void AvailBlockTableRebuild(struct AvailBlockTable * pTable, struct BlockWearTable * pWearTable, UINT64 blockNum);
UINT64 AvailBlockTableGetBlocks(struct AvailBlockTable * pTable, block_t * buffer, int blockNum);
void AvailBlockTablePutBlocks(struct AvailBlockTable * pTable, block_t block, UINT64 index);
void AvailBlockTableUninit(struct AvailBlockTable * pTable);

#endif