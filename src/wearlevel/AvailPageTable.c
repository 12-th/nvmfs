#include "AvailPageTable.h"
#include "Config.h"
#include "Layouter.h"
#include "PageWearTable.h"
#include <linux/slab.h>

static inline void BitmapInit(UINT64 * bitmap)
{
    memset(bitmap, 0, sizeof(UINT64) * 8);
}

static inline void BitmapSet(UINT64 * bitmap, int index)
{
    bitmap[index >> 6] |= (1UL << (index & 63));
}

static inline void BitmapClear(UINT64 * bitmap, int index)
{
    bitmap[index >> 6] &= (~(1UL << (index & 63)));
}

static inline UINT64 BitmapGet(UINT64 * bitmap, int index)
{
    return bitmap[index >> 6] & (1UL << (index & 63));
}

static inline unsigned long ffz(unsigned long word)
{
    asm("rep; bsf %1,%0" : "=r"(word) : "r"(~word));
    return word;
}

static inline int BitmapZeroGet(UINT64 * bitmap)
{
    int i;
    for (i = 0; i < 8; ++i)
    {
        if (bitmap[i] != -1UL)
        {
            return i * 64 + ffz(bitmap[i]);
        }
    }
    return -1;
}

static void PageGroupInit(struct PageGroup * pGroup, struct PageWearTable * pTable, UINT32 wearCountThreshold,
                          physical_block_t block)
{
    int i;
    UINT32 * wearCounts;

    BitmapInit(pGroup->bitmap);
    wearCounts = kmalloc(sizeof(UINT32) * 512, GFP_KERNEL);
    PageWearTableBatchGet(pTable, block_to_page(block), wearCounts);
    for (i = 0; i < 512; ++i)
    {
        if (wearCounts[i] > wearCountThreshold)
        {
            BitmapSet(pGroup->bitmap, i);
        }
    }
    INIT_LIST_HEAD(&pGroup->lruList);
    pGroup->block = block;
    pGroup->threshold = wearCountThreshold;
    kfree(wearCounts);
}

void PageGroupThresholdIncrease(struct PageGroup * pGroup)
{
    pGroup->threshold += STEP_WEAR_COUNT;
    BitmapInit(pGroup->bitmap);
}

void AvailPageTableInit(struct AvailPageTable * pTable, UINT64 pageNum)
{
    pTable->hashTable = kzmalloc(sizeof(struct PageGroup *) * pageNum, GFP_KERNEL);
    pTable->count = pageNum;
    INIT_LIST_HEAD(&pTable->lruHead);
}

void AvailPageTableUninit(struct AvailPageTable * pTable)
{
    struct PageGroup *pos, *next;
    list_for_each_entry_safe(pos, next, &pTable->lruHead, lruList)
    {
        list_del(&pos->lruList);
        kfree(pos);
    }
    kfree(pTable->hashTable);
}

static struct PageGroup * PageGroupGet(struct AvailPageTable * pTable, struct PageWearTable * pageWearTable,
                                       UINT32 wearCountThreshold, physical_block_t block)
{
    struct PageGroup * pGroup;

    if (pTable->hashTable[block])
    {
        pGroup = pTable->hashTable[block];
        list_del(&pGroup->lruList);
        list_add(&pGroup->lruList, &pTable->lruHead);
        return pGroup;
    }

    pGroup = kmalloc(sizeof(struct PageGroup), GFP_KERNEL);
    PageGroupInit(pGroup, pageWearTable, wearCountThreshold, block);
    pTable->hashTable[block] = pGroup;
    pTable->count++;
    list_add(&pGroup->lruList, &pTable->lruHead);
    if (pTable->count > AVAIL_PAGE_TABLE_PAGE_GROUP_MAX_NUM)
    {
        struct PageGroup * pOutGroup = container_of(pTable->lruHead.prev, struct PageGroup, lruList);
        list_del(&pOutGroup->lruList);
        pTable->hashTable[pOutGroup->block] = NULL;
        kfree(pOutGroup);
        pTable->count--;
    }
    return pGroup;
}

static struct PageGroup * PageGroupTryGet(struct AvailPageTable * pTable, physical_block_t block)
{
    return pTable->hashTable[block];
}

static inline int RelativeIndexOfPage(physical_page_t page)
{
    return page & 511;
}

static inline physical_page_t RelativeIndexToPage(int index, physical_block_t block)
{
    return (block << (BITS_2M - BITS_4K)) + index;
}

int AvailPageTableSwapPrepare(struct AvailPageTable * pTable, struct PageWearTable * pageWearTable,
                              physical_page_t oldPage, physical_page_t * newPage, UINT32 wearCountThreshold)
{
    struct PageGroup * pGroup;
    physical_block_t block;
    int indexOfOldPage, indexOfNewPage;

    indexOfOldPage = RelativeIndexOfPage(oldPage);
    block = page_to_block(oldPage);
    pGroup = PageGroupGet(pTable, pageWearTable, wearCountThreshold, block);
    BitmapSet(pGroup->bitmap, indexOfOldPage);
    indexOfNewPage = BitmapZeroGet(pGroup->bitmap);
    if (indexOfNewPage == -1)
    {
        return 1;
    }

    BitmapSet(pGroup->bitmap, indexOfNewPage);
    *newPage = RelativeIndexToPage(indexOfNewPage, block);
    return 0;
}

void AvailPageTableSwapEnd(struct AvailPageTable * pTable, physical_page_t newPage, UINT32 newPageWearCount)
{
    struct PageGroup * pGroup;
    physical_block_t block;
    int indexOfNewPage;

    block = page_to_block(newPage);
    pGroup = PageGroupTryGet(pTable, block);
    if (!pGroup)
        return;
    indexOfNewPage = RelativeIndexOfPage(newPage);
    if (newPageWearCount <= pGroup->threshold)
    {
        BitmapClear(pGroup->bitmap, indexOfNewPage);
    }
}