#ifndef BLOCK_WEAR_TABLE
#define BLOCK_WEAR_TABLE

#include "Types.h"

struct NVMBlockWearTable
{
    UINT32 wearCounts[0];
};

struct BlockWearTable
{
    nvm_addr_t addr;
};

void BlockWearTableFormat(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum);
void BlockWearTableInit(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum);
UINT32 GetBlockWearCount(struct BlockWearTable * pTable, block_t block);
void UpdateBlockWearCount(struct BlockWearTable * pTable, block_t block, UINT32 newValue);

#endif