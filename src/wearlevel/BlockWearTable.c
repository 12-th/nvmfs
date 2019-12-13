#include "BlockWearTable.h"
#include "NVMOperations.h"
#include "common.h"

void BlockWearTableFormat(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    DEBUG_PRINT("block wear table format, start nvm addr is 0x%lx", addr);
    pTable->addr = addr;
    NVMemset(addr, 0, sizeof(UINT32) * blockNum);
}

void BlockWearTableInit(struct BlockWearTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    pTable->addr = addr;
}

void BlockWearTableUninit(struct BlockWearTable * pTable)
{
}

UINT32 BlockWearTableGet(struct BlockWearTable * pTable, physical_block_t block)
{
    return NVMRead32(pTable->addr + sizeof(UINT32) * block);
}

void BlockWearTableSet(struct BlockWearTable * pTable, physical_block_t block, UINT32 newValue)
{
    NVMWrite32(pTable->addr + sizeof(UINT32) * block, newValue);
}