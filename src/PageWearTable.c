#include "NVMOperations.h"
#include "PageWearTable.h"

void PageWearTableFormat(struct PageWearTable * pTable, nvm_addr_t addr, UINT64 pageNum)
{
    pTable->addr = addr;
    NVMemset(addr, 0, sizeof(UINT32) * pageNum);
}

void PageWearTableInit(struct PageWearTable * pTable, nvm_addr_t addr, UINT64 pageNum)
{
    pTable->addr = addr;
}

UINT32 PageWearTableGet(struct PageWearTable * pTable, UINT64 pageSeq)
{
    return NVMRead32(pTable->addr + sizeof(UINT32) * pageSeq);
}

void PageWearCountSet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 newWearCount)
{
    NVMWrite32(pTable->addr + sizeof(UINT32) * pageSeq, newWearCount);
}

void PageWearTableBatchGet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 * wearCountsArray)
{
    NVMRead(pTable->addr + sizeof(UINT32) * pageSeq, sizeof(UINT32) * 512, wearCountsArray);
}

void PageWearTableBatchSet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 wearCount)
{
    int i;
    nvm_addr_t addr;

    addr = pTable->addr + sizeof(UINT32) * pageSeq;
    for (i = 0; i < 512; ++i)
    {
        NVMWrite32(addr, wearCount);
        addr += sizeof(UINT32);
    }
}

void PageWearTableUninit(struct PageWearTable * pTable)
{
}
