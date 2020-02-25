#include "NVMOperations.h"
#include "PageManager.h"
#include "Types.h"
#include "WearLeveler.h"
#include <linux/mm.h>
#include <linux/slab.h>

struct PageLockHandle
{
    logic_addr_t addr[2];
};

void PageManagerInit(struct PageManager * manager, struct WearLeveler * wl)
{
    struct Layouter * l = &wl->layouter;
    manager->dataStartOffset = l->dataStart;
    manager->blockManager = &wl->blockManager;
    manager->unmapTable = &wl->pageUnmapTable;
    manager->blockUnmapTable = &wl->blockUnmapTable;
    manager->wearTable = &wl->pageWearTable;
    manager->log = &wl->pageSwapLog;
    manager->coldness = &wl->pageColdnessManager;
    manager->swapArea = &wl->pageSwapArea;
    manager->mapTable = &wl->mapTable;
    manager->swapLogLogicAddr = l->pageSwapLogLogicAddr;
    manager->swapAreaLogicAddr = l->pageSwapAreaLogicAddr;
}

void PageManagerRead(struct PageManager * manager, logic_addr_t addr, UINT64 size, void * buffer)
{
    struct MapTableLockHandle handle;
    nvm_addr_t physAddr;

    do
    {
        handle = MapTableLockPageForRead(manager->mapTable, addr);
        physAddr = MapTableQuery(manager->mapTable, addr);
        NVMRead(physAddr, size, buffer);
    } while (MapTableUnlockPageForRead(manager->mapTable, addr, handle));
}

static logic_addr_t LockOneBlock(struct PageManager * manager, physical_block_t block)
{
    nvm_addr_t addr = block_to_nvm_addr(block, manager->dataStartOffset);
    struct BlockInfo info;
    logic_addr_t logicAddr;

    do
    {
        BlockUnmapTableGet(manager->blockUnmapTable, block, &info);
        logicAddr = logical_block_to_addr(info.unmapBlock);
    } while (MapTableLockBlockForPageSwap(manager->mapTable, logicAddr, addr));

    return logicAddr;
}

static logic_addr_t LockOnePage(struct PageManager * manager, physical_page_t page)
{
    logic_addr_t logicAddr;
    nvm_addr_t physAddr;

    physAddr = page_to_nvm_addr(page, manager->dataStartOffset);
    do
    {
        logical_page_t logicPage = PageUnmapTableGet(manager->unmapTable, page);
        logicAddr = logical_page_to_addr(logicPage);
    } while (MapTableLockPageForSwap(manager->mapTable, logicAddr, physAddr));

    return logicAddr;
}

static struct PageLockHandle LockPageForSwap(struct PageManager * manager, physical_page_t pageA, physical_page_t pageB)
{
    struct PageLockHandle handle;

    LockOneBlock(manager, page_to_block(pageA));
    if (pageA < pageB)
    {
        handle.addr[0] = LockOnePage(manager, pageA);
        handle.addr[1] = LockOnePage(manager, pageB);
    }
    else
    {
        handle.addr[0] = LockOnePage(manager, pageB);
        handle.addr[1] = LockOnePage(manager, pageA);
    }
    return handle;
}

static void UnlockPageForSwap(struct PageManager * manager, struct PageLockHandle handle)
{
    MapTableUnlockPageForSwap(manager->mapTable, handle.addr[0]);
    MapTableUnlockPageForSwap(manager->mapTable, handle.addr[1]);
    MapTableUnlockBlockForPageSwap(manager->mapTable, handle.addr[0]);
}

static void SwapPageFast(struct PageManager * manager, physical_page_t pageA, physical_page_t pageB)
{
    nvm_addr_t logAreaAddr;
    logical_page_t logicPageA, logicPageB;
    UINT32 wcA, wcB;

    logicPageA = PageUnmapTableGet(manager->unmapTable, pageA);
    logicPageB = PageUnmapTableGet(manager->unmapTable, pageB);
    logAreaAddr = LogPageSwapBegin(manager->log, pageA, pageB, invalid_nvm_addr, logicPageA, logicPageB, 1);
    NVMemcpy(page_to_nvm_addr(pageB, manager->dataStartOffset), page_to_nvm_addr(pageA, manager->dataStartOffset),
             SIZE_4K);
    LogPageSwapCommitStepx(logAreaAddr, 2);
    PageUnmapTableSet(manager->unmapTable, pageA, logicPageB);
    PageUnmapTableSet(manager->unmapTable, pageB, logicPageA);
    MapTablePageSwap(manager->mapTable, logicPageA, logicPageB);
    LogPageSwapEnd(logAreaAddr);

    wcA = PageWearTableGet(manager->wearTable, pageA);
    wcB = PageWearTableGet(manager->wearTable, pageB);
    PageWearTableSet(manager->wearTable, pageA, wcA + 1);
    PageWearTableSet(manager->wearTable, pageB, wcB + 1);
    if (wcB + 1 < STEP_WEAR_COUNT)
    {
        PageColdnessManagerSwapPut(manager->coldness, pageB);
    }
}

static void SwapPageSlow(struct PageManager * manager, physical_page_t pageA, physical_page_t pageB)
{
    nvm_addr_t logAreaAddr, swapTmpArea;
    logical_page_t logicPageA, logicPageB;
    UINT32 wcA, wcB;

    logicPageA = PageUnmapTableGet(manager->unmapTable, pageA);
    logicPageB = PageUnmapTableGet(manager->unmapTable, pageB);
    swapTmpArea = PageSwapAreaGetTmpArea(manager->swapArea);
    logAreaAddr = LogPageSwapBegin(manager->log, pageA, pageB, swapTmpArea, logicPageA, logicPageB, 1);
    NVMemcpy(swapTmpArea, page_to_nvm_addr(pageB, manager->dataStartOffset), SIZE_4K);
    LogPageSwapCommitStepx(logAreaAddr, 2);
    NVMemcpy(page_to_nvm_addr(pageB, manager->dataStartOffset), page_to_nvm_addr(pageA, manager->dataStartOffset),
             SIZE_4K);
    LogPageSwapCommitStepx(logAreaAddr, 3);
    NVMemcpy(page_to_nvm_addr(pageA, manager->dataStartOffset), swapTmpArea, SIZE_4K);
    LogPageSwapCommitStepx(logAreaAddr, 4);
    PageUnmapTableSet(manager->unmapTable, pageA, logicPageB);
    PageUnmapTableSet(manager->unmapTable, pageB, logicPageA);
    MapTablePageSwap(manager->mapTable, logicPageA, logicPageB);
    LogPageSwapEnd(logAreaAddr);

    wcA = PageWearTableGet(manager->wearTable, pageA);
    wcB = PageWearTableGet(manager->wearTable, pageB);
    PageWearTableSet(manager->wearTable, pageA, wcA + 1);
    PageWearTableSet(manager->wearTable, pageB, wcB + 1);
    if (wcB + 1 < STEP_WEAR_COUNT)
    {
        PageColdnessManagerSwapPut(manager->coldness, pageB);
    }
}

static void SwapPage(struct PageManager * manager, nvm_addr_t physAddr)
{

    int shouldSwapBlock;
    struct PageLockHandle handle;
    logic_addr_t newPageLogicAddr;
    UINT32 threshold;
    physical_page_t oldPage, newPage;
    int tmpAreaUsed = 0;

    oldPage = nvm_addr_to_page(physAddr, manager->dataStartOffset);
    PageColdnessManagerSwapPrepare(manager->coldness, manager->wearTable, oldPage, &newPage, &shouldSwapBlock);
    if (shouldSwapBlock)
    {
        nvm_addr_t blockAddr;
        UINT32 * wearCounts;

        wearCounts = kmalloc(sizeof(UINT32) * 512, GFP_KERNEL);
        blockAddr = physAddr & (~((1UL << BITS_2M) - 1));
        BlockManagerIncreaseWearCount(manager->blockManager, blockAddr, STEP_WEAR_COUNT);
        PageWearTableBatchIncreaseThreshold(manager->wearTable, nvm_addr_to_page(blockAddr, manager->dataStartOffset),
                                            wearCounts);
        PageColdnessManagerIncreaseThreshold(manager->coldness, nvm_addr_to_block(blockAddr, manager->dataStartOffset),
                                             wearCounts);
        kfree(wearCounts);
        return;
    }

    handle = LockPageForSwap(manager, oldPage, newPage);

    newPageLogicAddr = logical_page_to_addr(PageUnmapTableGet(manager->unmapTable, newPage));
    if (!MapTableIsPageBusy(manager->mapTable, newPageLogicAddr))
    {
        SwapPageFast(manager, oldPage, newPage);
    }
    else
    {
        SwapPageSlow(manager, oldPage, newPage);
        tmpAreaUsed = 1;
    }

    UnlockPageForSwap(manager, handle);
    PageColdnessManagerSwapPut(manager->coldness, newPage);

    threshold = manager->blockManager->threshold;
    if (PageSwapLogIsFull(manager->log))
    {
        nvm_addr_t newPhysAddr;
        BlockManagerIncreaseSpecialBlockWearCount(manager->blockManager, threshold,
                                                  PageSwapLogGetAreaAddr(manager->log), &newPhysAddr);
        if (newPhysAddr != invalid_nvm_addr)
            PageSwapLogSetAreaAddr(manager->log, newPhysAddr);
    }
    if (tmpAreaUsed && PageSwapAreaIsFull(manager->swapArea))
    {
        nvm_addr_t newPhysAddr;
        BlockManagerIncreaseSpecialBlockWearCount(manager->blockManager, threshold,
                                                  PageSwapAreaGetAddr(manager->swapArea), &newPhysAddr);
        if (newPhysAddr != invalid_nvm_addr)
            PageSwapAreaSetAddr(manager->swapArea, newPhysAddr);
    }
}

static void PageManagerIncreaseWearCount(struct PageManager * manager, nvm_addr_t physAddr, int increaseWearCount)
{
    if (increaseWearCount)
    {
        UINT32 wc;

        wc = PageWearTableGet(manager->wearTable, nvm_addr_to_page(physAddr, manager->dataStartOffset));
        PageWearTableSet(manager->wearTable, nvm_addr_to_page(physAddr, manager->dataStartOffset),
                         wc + increaseWearCount);
        if (wc + increaseWearCount >= STEP_WEAR_COUNT)
        {
            SwapPage(manager, physAddr);
        }
    }
}

void PageManagerWrite(struct PageManager * manager, logic_addr_t addr, UINT64 size, void * buffer,
                      int increaseWearCount)
{
    nvm_addr_t physAddr;

    MapTableLockPageForWrite(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    NVMWrite(physAddr, size, buffer);
    MapTablePageBusy(manager->mapTable, addr);
    MapTableUnlockPageForWrite(manager->mapTable, addr);

    PageManagerIncreaseWearCount(manager, physAddr, increaseWearCount);
}

void PageManagerMemset(struct PageManager * manager, logic_addr_t addr, UINT64 size, int value, int increaseWearCount)
{
    nvm_addr_t physAddr;

    MapTableLockPageForWrite(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    NVMemset(physAddr, value, size);
    MapTablePageBusy(manager->mapTable, addr);
    MapTableUnlockPageForWrite(manager->mapTable, addr);

    PageManagerIncreaseWearCount(manager, physAddr, increaseWearCount);
}

void PageManagerTrim(struct PageManager * manager, logic_addr_t addr)
{
    MapTableLockPageForWrite(manager->mapTable, addr);
    MapTablePageTrim(manager->mapTable, addr);
    MapTableUnlockPageForWrite(manager->mapTable, addr);
}