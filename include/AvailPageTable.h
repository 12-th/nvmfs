#ifndef AVIAL_PAGE_TABLE_H
#define AVIAL_PAGE_TABLE_H

#include "Types.h"
#include <linux/list.h>

struct PageWearTable;

struct PageGroup
{
    UINT64 bitmap[8];
    struct list_head lruList;
    physical_block_t block;
    UINT32 threshold;
};

struct AvailPageTable
{
    struct list_head lruHead;
    struct PageGroup ** hashTable;
    UINT32 count;
};

void PageGroupThresholdIncrease(struct PageGroup * pGroup);
void AvailPageTableInit(struct AvailPageTable * pTable, UINT64 pageNum);
void AvailPageTableUninit(struct AvailPageTable * pTable);
int AvailPageTableSwapPrepare(struct AvailPageTable * pTable, struct PageWearTable * pageWearTable,
                              physical_page_t oldPage, physical_page_t * newPage, UINT32 wearCountThreshold);
void AvailPageTableSwapEnd(struct AvailPageTable * pTable, physical_page_t newPage, UINT32 newPageWearCount);

#endif