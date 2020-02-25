#include "Config.h"
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

void PageWearTableSet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 newWearCount)
{
    NVMWrite32(pTable->addr + sizeof(UINT32) * pageSeq, newWearCount);
}

void PageWearTableBatchGet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 * wearCountsArray)
{
    NVMRead(pTable->addr + sizeof(UINT32) * pageSeq, sizeof(UINT32) * 512, wearCountsArray);
}

UINT32 PageWearTableSumGet(struct PageWearTable * pTable, UINT64 pageSeq)
{
    int i;
    UINT32 sum = 0;
    for (i = 0; i < 512; ++i)
    {
        sum += PageWearTableGet(pTable, pageSeq + i);
    }
    return sum;
}

void PageWearTableBatchFormat(struct PageWearTable * pTable, UINT64 pageSeq)
{
    nvm_addr_t addr;

    addr = pTable->addr + sizeof(UINT32) * pageSeq;
    NVMemset(addr, 0, sizeof(UINT32) * 512);
}

void PageWearTableBatchIncreaseThreshold(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 * wearCounts)
{
    int i;

    PageWearTableBatchGet(pTable, pageSeq, wearCounts);
    for (i = 0; i < 512; ++i)
    {
        ASSERT(wearCounts[i] >= STEP_WEAR_COUNT);
        wearCounts[i] -= STEP_WEAR_COUNT;
        PageWearTableSet(pTable, pageSeq + i, wearCounts[i]);
    }
}

void PageWearTableUninit(struct PageWearTable * pTable)
{
}