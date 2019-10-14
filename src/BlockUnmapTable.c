#include "BlockUnmapTable.h"
#include "NVMOperations.h"

void BlockUnmapTableFormat(struct BlockUnmapTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    UINT32 i;

    pTable->addr = addr;
    for (i = 0; i < blockNum; ++i)
    {
        struct BlockInfo info = {.unmapBlock = i, .busy = 0, .fineGrain = 0};
        NVMWrite(addr, sizeof(info), &info);
        addr += sizeof(struct BlockInfo);
    }
}

void BlockUnmapTableInit(struct BlockUnmapTable * pTable, nvm_addr_t addr, UINT64 blockNum)
{
    pTable->addr = addr;
}

void BlockUnmapTableGet(struct BlockUnmapTable * pTable, physical_block_t block, struct BlockInfo * info)
{
    NVMRead(pTable->addr + offsetof(struct NVMBlockUnmapTable, infos[block]), sizeof(struct BlockInfo), info);
}

void BlockUnmapTableSet(struct BlockUnmapTable * pTable, physical_block_t block, struct BlockInfo * info)
{
    NVMWrite(pTable->addr + offsetof(struct NVMBlockUnmapTable, infos[block]), sizeof(struct BlockInfo), info);
}

void BlockUnmapTableUninit(struct BlockUnmapTable * pTable)
{
}