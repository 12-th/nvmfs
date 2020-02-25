#include "Config.h"
#include "Layouter.h"
#include "PageColdnessManager.h"
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

static inline unsigned long PageColdnessManagerFfz(unsigned long word)
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
            return i * 64 + PageColdnessManagerFfz(bitmap[i]);
        }
    }
    return -1;
}

static void PageGroupInit(struct PageGroup * pGroup, struct PageWearTable * manager, physical_block_t block)
{
    int i;
    UINT32 * wearCounts;

    BitmapInit(pGroup->bitmap);
    wearCounts = kmalloc(sizeof(UINT32) * 512, GFP_KERNEL);
    PageWearTableBatchGet(manager, block_to_page(block), wearCounts);
    for (i = 0; i < 512; ++i)
    {
        if (wearCounts[i] > STEP_WEAR_COUNT)
        {
            BitmapSet(pGroup->bitmap, i);
        }
    }
    INIT_LIST_HEAD(&pGroup->lruList);
    pGroup->block = block;
    kfree(wearCounts);
}

void PageColdnessManagerFormat(struct PageColdnessManager * manager, UINT64 pageNum)
{
    manager->hashTable = kzalloc(sizeof(struct PageGroup *) * pageNum, GFP_KERNEL);
    manager->count = 0;
    INIT_LIST_HEAD(&manager->lruHead);
}

void PageColdnessManagerUninit(struct PageColdnessManager * manager)
{
    struct PageGroup *pos, *next;
    list_for_each_entry_safe(pos, next, &manager->lruHead, lruList)
    {
        list_del(&pos->lruList);
        kfree(pos);
    }
    kfree(manager->hashTable);
}

static struct PageGroup * PageGroupGet(struct PageColdnessManager * manager, struct PageWearTable * pageWearTable,
                                       physical_block_t block)
{
    struct PageGroup * pGroup;

    if (manager->hashTable[block])
    {
        pGroup = manager->hashTable[block];
        list_del(&pGroup->lruList);
        list_add(&pGroup->lruList, &manager->lruHead);
        return pGroup;
    }

    pGroup = kmalloc(sizeof(struct PageGroup), GFP_KERNEL);
    PageGroupInit(pGroup, pageWearTable, block);
    manager->hashTable[block] = pGroup;
    manager->count++;
    list_add(&pGroup->lruList, &manager->lruHead);
    if (manager->count > AVAIL_PAGE_TABLE_PAGE_GROUP_MAX_NUM)
    {
        struct PageGroup * pOutGroup = container_of(manager->lruHead.prev, struct PageGroup, lruList);
        list_del(&pOutGroup->lruList);
        manager->hashTable[pOutGroup->block] = NULL;
        kfree(pOutGroup);
        manager->count--;
    }
    return pGroup;
}

static struct PageGroup * PageGroupTryGet(struct PageColdnessManager * manager, physical_block_t block)
{
    return manager->hashTable[block];
}

static inline int RelativeIndexOfPage(physical_page_t page)
{
    return page & 511;
}

static inline physical_page_t RelativeIndexToPage(int index, physical_block_t block)
{
    return (block << (BITS_2M - BITS_4K)) + index;
}

void PageColdnessManagerSwapPrepare(struct PageColdnessManager * manager, struct PageWearTable * pageWearTable,
                                    physical_page_t oldPage, physical_page_t * newPage, int * shouldSwapBlock)
{
    struct PageGroup * pGroup;
    physical_block_t block;
    int indexOfOldPage, indexOfNewPage;

    indexOfOldPage = RelativeIndexOfPage(oldPage);
    block = page_to_block(oldPage);
    pGroup = PageGroupGet(manager, pageWearTable, block);
    BitmapSet(pGroup->bitmap, indexOfOldPage);
    indexOfNewPage = BitmapZeroGet(pGroup->bitmap);
    if (indexOfNewPage == -1)
    {
        *shouldSwapBlock = 1;
        return;
    }

    BitmapSet(pGroup->bitmap, indexOfNewPage);
    *newPage = RelativeIndexToPage(indexOfNewPage, block);
    *shouldSwapBlock = 0;
}

void PageColdnessManagerSwapPut(struct PageColdnessManager * manager, physical_page_t newPage)
{
    struct PageGroup * pGroup;
    physical_block_t block;
    int indexOfNewPage;

    block = page_to_block(newPage);
    pGroup = PageGroupTryGet(manager, block);
    if (!pGroup)
        return;
    indexOfNewPage = RelativeIndexOfPage(newPage);
    BitmapClear(pGroup->bitmap, indexOfNewPage);
}

void PageColdnessManagerIncreaseThreshold(struct PageColdnessManager * manager, physical_block_t block,
                                          UINT32 * wearCounts)
{
    struct PageGroup * pGroup;
    int i;

    pGroup = PageGroupTryGet(manager, block);
    if (!pGroup)
        return;
    BitmapInit(pGroup->bitmap);

    for (i = 0; i < 512; i++)
    {
        if (wearCounts[i] >= STEP_WEAR_COUNT)
        {
            BitmapClear(pGroup->bitmap, i);
        }
    }
}