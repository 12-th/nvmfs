#ifndef PAGE_WEAR_TABLE_H
#define PAGE_WEAR_TABLE_H

#include "Types.h"

struct NVMPageWearTable
{
    UINT32 wearCounts[0];
};

struct PageWearTable
{
    nvm_addr_t addr;
};

void PageWearTableFormat(struct PageWearTable * pTable, nvm_addr_t addr, UINT64 pageNum);
void PageWearTableInit(struct PageWearTable * pTable, nvm_addr_t addr, UINT64 pageNum);
void PageWearTableUninit(struct PageWearTable * pTable);
UINT32 PageWearTableGet(struct PageWearTable * pTable, UINT64 pageSeq);
void PageWearTableSet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 newWearCount);
void PageWearTableBatchGet(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 * wearCountsArray);
void PageWearTableBatchFormat(struct PageWearTable * pTable, UINT64 pageSeq);
void PageWearTableBatchIncreaseThreshold(struct PageWearTable * pTable, UINT64 pageSeq, UINT32 * wearCounts);
UINT32 PageWearTableSumGet(struct PageWearTable * pTable, UINT64 pageSeq);

#endif