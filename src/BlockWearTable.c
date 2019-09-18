#include "BlockWearTable.h"
#include "NVMOperations.h"

void BlockWearTableFormat(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    pTable->addr = addr;
    NVMemset(addr, 0, sizeof(UINT32) * blockNum);
}

void BlockWearTableInit(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    pTable->addr = addr;
}

UINT32 GetBlockWearCount(struct BlockWearTable * pTable, block_t block)
{
    return NVMRead32(pTable->addr + sizeof(UINT32) * block);
}

void UpdateBlockWearCount(struct BlockWearTable * pTable, block_t block, UINT32 newValue)
{
    NVMWrite32(pTable->addr + sizeof(UINT32) * block, newValue);
}