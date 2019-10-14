#ifndef BLOCK_WEAR_TABLE
#define BLOCK_WEAR_TABLE

#include "Config.h"
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
void BlockWearTableUninit(struct BlockWearTable * pTable);
UINT32 GetBlockWearCount(struct BlockWearTable * pTable, physical_block_t block);
void UpdateBlockWearCount(struct BlockWearTable * pTable, physical_block_t block, UINT32 newValue);

static inline int IsCrossWearCountThreshold(UINT32 oldWearCount, UINT32 newWearCount)
{
    return !(oldWearCount / STEP_WEAR_COUNT == newWearCount / STEP_WEAR_COUNT);
}

#endif