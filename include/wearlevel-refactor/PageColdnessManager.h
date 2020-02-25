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
};

struct PageColdnessManager
{
    struct list_head lruHead;
    struct PageGroup ** hashTable;
    UINT32 count;
};

void PageGroupThresholdIncrease(struct PageGroup * pGroup);
void PageColdnessManagerFormat(struct PageColdnessManager * manager, UINT64 pageNum);
void PageColdnessManagerUninit(struct PageColdnessManager * manager);
void PageColdnessManagerSwapPrepare(struct PageColdnessManager * manager, struct PageWearTable * pageWearTable,
                                    physical_page_t oldPage, physical_page_t * newPage, int * shouldSwapBlock);
void PageColdnessManagerSwapPut(struct PageColdnessManager * manager, physical_page_t newPage);
void PageColdnessManagerIncreaseThreshold(struct PageColdnessManager * manager, physical_block_t block,
                                          UINT32 * wearCounts);

#endif