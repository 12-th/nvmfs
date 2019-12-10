#ifndef BLOCK_UNMAP_TABLE_H
#define BLOCK_UNMAP_TABLE_H

#include "Types.h"

struct BlockInfo
{
    logical_block_t unmapBlock : 30;
    int busy : 1;
    int fineGrain : 1;
};

struct NVMBlockUnmapTable
{
    struct BlockInfo infos[0];
};

struct BlockUnmapTable
{
    nvm_addr_t addr;
};

void BlockUnmapTableFormat(struct BlockUnmapTable * pTable, nvm_addr_t addr, UINT64 blockNum);
void BlockUnmapTableInit(struct BlockUnmapTable * pTable, nvm_addr_t addr, UINT64 blockNum);
void BlockUnmapTableGet(struct BlockUnmapTable * pTable, physical_block_t block, struct BlockInfo * info);
void BlockUnmapTableSet(struct BlockUnmapTable * pTable, physical_block_t block, struct BlockInfo * info);
void BlockUnmapTableUninit(struct BlockUnmapTable * pTable);

void BlockUnmapTableRecoveryBegin(struct BlockUnmapTable * pTable, nvm_addr_t addr, UINT64 blockNum);
void BlockUnmapTableRecoverySet(struct BlockUnmapTable * pTable, physical_block_t block, struct BlockInfo * info);
void BlockUnmapTableRecoveryEnd(struct BlockUnmapTable * pTable);

#endif