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

UINT32 GetPageWearCount(struct PageWearTable * pTable, UINT64 pageSeq)
{
    return NVMRead8(pTable->addr + sizeof(UINT32) * pageSeq);
}

void PageWearTableUninit(struct PageWearTable * pTable)
{
}