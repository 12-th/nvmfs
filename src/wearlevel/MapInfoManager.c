#include "BlockWearTable.h"
#include "Layouter.h"
#include "MapInfoManager.h"
#include "NVMOperations.h"
#include "PageWearTable.h"
#include <linux/slab.h>

void MapInfoManagerFormat(struct MapInfoManager * manager, struct Layouter * layouter)
{
    manager->dataStartOffset = DataStartAddrQuery(layouter);
    BlockUnmapTableFormat(&manager->blockUnmapTable, BlockUnmapTableAddrQuery(layouter), BlockNumQuery(layouter));
    PageUnmapTableFormat(&manager->pageUnmapTable, &manager->blockUnmapTable, PageUnmapTableAddrQuery(layouter),
                         PageNumQuery(layouter), PageUnmapTableSizeQuery(layouter));
    NVMAccessControllerRebuild(&manager->nvmAccessController, &manager->blockUnmapTable, &manager->pageUnmapTable,
                               BlockNumQuery(layouter), DataStartAddrQuery(layouter));
}

void MapInfoManagerInit(struct MapInfoManager * manager, struct Layouter * layouter)
{
    manager->dataStartOffset = DataStartAddrQuery(layouter);
    BlockUnmapTableInit(&manager->blockUnmapTable, BlockUnmapTableAddrQuery(layouter), BlockNumQuery(layouter));
    PageUnmapTableInit(&manager->pageUnmapTable, &manager->blockUnmapTable, PageUnmapTableAddrQuery(layouter),
                       PageNumQuery(layouter));
    NVMAccessControllerRebuild(&manager->nvmAccessController, &manager->blockUnmapTable, &manager->pageUnmapTable,
                               BlockNumQuery(layouter), DataStartAddrQuery(layouter));
}

void MapInfoManagerUninit(struct MapInfoManager * manager)
{
    NVMAccessControllerUninit(&manager->nvmAccessController);
    PageUnmapTableUninit(&manager->pageUnmapTable);
    BlockUnmapTableUninit(&manager->blockUnmapTable);
}

void PageMapInfoBuildFromPhysicalPage(struct PageMapInfo * info, struct MapInfoManager * manager, physical_page_t page)
{
    info->physPage = page;
    PageUnmapTableGet(&manager->pageUnmapTable, page, &info->pageInfo);
}

void PageMapInfoBuildFromLogicalPage(struct PageMapInfo * info, struct MapInfoManager * manager, logical_page_t page)
{
    nvm_addr_t addr;

    addr = NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, logical_page_to_addr(page));
    info->physPage = nvm_addr_to_page(addr, manager->dataStartOffset);
    PageUnmapTableGet(&manager->pageUnmapTable, info->physPage, &info->pageInfo);

    ASSERT(info->pageInfo.unmapPage == page);
}

void BlockMapInfoBuildFromPhysicalBlock(struct BlockMapInfo * info, struct MapInfoManager * manager,
                                        physical_block_t block)
{
    info->physBlock = block;
    BlockUnmapTableGet(&manager->blockUnmapTable, block, &info->blockInfo);
}

void BlockMapInfoBuildFromLogicalBlock(struct BlockMapInfo * info, struct MapInfoManager * manager,
                                       logical_block_t block)
{
    nvm_addr_t addr;

    addr = NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, logical_block_to_addr(block));
    info->physBlock = nvm_addr_to_block(addr, manager->dataStartOffset);
    BlockUnmapTableGet(&manager->blockUnmapTable, info->physBlock, &info->blockInfo);

    ASSERT(info->blockInfo.unmapBlock == block);
}

void BlockWearCountIncreasePrepare(struct MapInfoManager * manager, logic_addr_t addr)
{
    NVMAccessControllerBlockUniqueLock(&manager->nvmAccessController, addr);
}

void BlockWearCountIncreaseEnd(struct MapInfoManager * manager, logic_addr_t addr)
{
    NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, addr);
}

void PageWearCountIncreasePrepare(struct MapInfoManager * manager, logic_addr_t addr)
{
    NVMAccessControllerBlockUniqueLock(&manager->nvmAccessController, addr);
    NVMAccessControllerPageLock(&manager->nvmAccessController, addr);
}

void PageWearCountIncreaseEnd(struct MapInfoManager * manager, logic_addr_t addr)
{
    NVMAccessControllerPageUnlock(&manager->nvmAccessController, addr);
    NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, addr);
}

static inline void PageLock(struct MapInfoManager * manager, physical_page_t page, struct PageMapInfo * mapInfo)
{
    struct PageInfo info;
    logic_addr_t addr;

again:
    PageUnmapTableGet(&manager->pageUnmapTable, page, &info);
    addr = logical_page_to_addr(info.unmapPage);
    NVMAccessControllerPageLock(&manager->nvmAccessController, addr);
    if (NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr) !=
        page_to_nvm_addr(page, manager->dataStartOffset))
    {
        NVMAccessControllerPageUnlock(&manager->nvmAccessController, addr);
        goto again;
    }
    mapInfo->physPage = page;
    PageUnmapTableGet(&manager->pageUnmapTable, page, &mapInfo->pageInfo);
}

void PageWearCountIncreaseToSwapPrepare(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo,
                                        struct PageMapInfo * newPageInfo, physical_page_t oldPage,
                                        physical_page_t newPage)
{
    if (oldPage < newPage)
    {
        PageLock(manager, newPage, newPageInfo);
    }
    else
    {
        NVMAccessControllerPageUnlock(&manager->nvmAccessController,
                                      logical_page_to_addr(oldPageInfo->pageInfo.unmapPage));
        PageLock(manager, newPage, newPageInfo);
        PageLock(manager, oldPage, oldPageInfo);
    }
}

void PageSwapEnd(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo, struct PageMapInfo * newPageInfo)
{
    logic_addr_t oldPageAddr, newPageAddr;
    newPageAddr = logical_page_to_addr(newPageInfo->pageInfo.unmapPage);
    oldPageAddr = logical_page_to_addr(oldPageInfo->pageInfo.unmapPage);
    if (oldPageInfo->physPage < newPageInfo->physPage)
    {
        NVMAccessControllerPageUnlock(&manager->nvmAccessController, newPageAddr);
        NVMAccessControllerPageUnlock(&manager->nvmAccessController, oldPageAddr);
    }
    else
    {
        NVMAccessControllerPageUnlock(&manager->nvmAccessController, oldPageAddr);
        NVMAccessControllerPageUnlock(&manager->nvmAccessController, newPageAddr);
    }
    NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, oldPageAddr);
}

static inline void BlockLock(struct MapInfoManager * manager, physical_block_t block, struct BlockMapInfo * mapInfo)
{
    struct BlockInfo info;
    logic_addr_t addr;

again:
    BlockUnmapTableGet(&manager->blockUnmapTable, block, &info);
    addr = logical_block_to_addr(info.unmapBlock);
    NVMAccessControllerBlockUniqueLock(&manager->nvmAccessController, addr);
    if (NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr) !=
        block_to_nvm_addr(block, manager->dataStartOffset))
    {
        NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, addr);
        goto again;
    }
    mapInfo->physBlock = block;
    BlockUnmapTableGet(&manager->blockUnmapTable, block, &mapInfo->blockInfo);
}

void BlockSwapPrepare(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                      struct BlockMapInfo * newBlockInfo, physical_block_t oldBlock, physical_block_t newBlock)
{
    ASSERT(oldBlock != newBlock);
    if (oldBlock < newBlock)
    {
        BlockLock(manager, oldBlock, oldBlockInfo);
        BlockLock(manager, newBlock, newBlockInfo);
    }
    else
    {
        BlockLock(manager, newBlock, newBlockInfo);
        BlockLock(manager, oldBlock, oldBlockInfo);
    }
}

void BlockSwapEnd(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                  struct BlockMapInfo * newBlockInfo)
{
    if (oldBlockInfo->blockInfo.unmapBlock < newBlockInfo->blockInfo.unmapBlock)
    {
        NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController,
                                             logical_block_to_addr(newBlockInfo->blockInfo.unmapBlock));
        NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController,
                                             logical_block_to_addr(oldBlockInfo->blockInfo.unmapBlock));
    }
    else
    {
        NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController,
                                             logical_block_to_addr(oldBlockInfo->blockInfo.unmapBlock));
        NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController,
                                             logical_block_to_addr(newBlockInfo->blockInfo.unmapBlock));
    }
}

void MapInfoManagerSwapBlock(struct MapInfoManager * manager, struct BlockMapInfo * oldBlockInfo,
                             struct BlockMapInfo * newBlockInfo)
{
    BlockUnmapTableSet(&manager->blockUnmapTable, oldBlockInfo->physBlock, &newBlockInfo->blockInfo);
    BlockUnmapTableSet(&manager->blockUnmapTable, newBlockInfo->physBlock, &oldBlockInfo->blockInfo);
    NVMAccessControllerBlockSwapMap(&manager->nvmAccessController, oldBlockInfo->blockInfo.unmapBlock,
                                    newBlockInfo->blockInfo.unmapBlock);
}

void MapInfoManagerSwapPage(struct MapInfoManager * manager, struct PageMapInfo * oldPageInfo,
                            struct PageMapInfo * newPageInfo)
{
    PageUnmapTableSet(&manager->pageUnmapTable, oldPageInfo->physPage, &newPageInfo->pageInfo);
    PageUnmapTableSet(&manager->pageUnmapTable, newPageInfo->physPage, &oldPageInfo->pageInfo);
    NVMAccessControllerPageSwapMap(&manager->nvmAccessController, oldPageInfo->pageInfo.unmapPage,
                                   newPageInfo->pageInfo.unmapPage);
}

static inline UINT32 CalculateBlockMaxReadWriteSize(logic_addr_t addr, UINT32 size)
{
    UINT32 restSize;

    restSize = SIZE_2M - (addr & ((1UL << BITS_2M) - 1));
    if (size > restSize)
        return restSize;
    return size;
}

static inline UINT32 CalculatePageMaxReadWriteSize(logic_addr_t addr, UINT32 size)
{
    UINT32 restSize;

    restSize = SIZE_4K - (addr & ((1UL << BITS_4K) - 1));
    if (size > restSize)
        return restSize;
    return size;
}

static inline UINT32 CalculateMaxReadWriteSize(logic_addr_t addr, UINT32 size, int isPage)
{
    if (isPage)
        return CalculatePageMaxReadWriteSize(addr, size);
    return CalculateBlockMaxReadWriteSize(addr, size);
}

UINT32 MapInfoManagerRead(struct MapInfoManager * manager, logic_addr_t addr, void * buffer, UINT32 size)
{
    UINT32 readSize;

    NVMAccessControllerSharedLock(&manager->nvmAccessController, addr);
    readSize =
        CalculateMaxReadWriteSize(addr, size, NVMAccessControllerIsBlockSplited(&manager->nvmAccessController, addr));
    NVMRead(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr), readSize, buffer);
    NVMAccessControllerSharedUnlock(&manager->nvmAccessController, addr);
    return readSize;
}

UINT32 MapInfoManagerWrite(struct MapInfoManager * manager, logic_addr_t addr, void * buffer, UINT32 size)
{
    UINT32 readSize;

    NVMAccessControllerSharedLock(&manager->nvmAccessController, addr);
    readSize =
        CalculateMaxReadWriteSize(addr, size, NVMAccessControllerIsBlockSplited(&manager->nvmAccessController, addr));
    NVMWrite(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr), readSize, buffer);
    NVMAccessControllerSharedUnlock(&manager->nvmAccessController, addr);
    return readSize;
}

void MapInfoManagerTrim(struct MapInfoManager * manager, logic_addr_t addr)
{
    int shouldModifyNVMData;

    NVMAccessControllerUniqueLock(&manager->nvmAccessController, addr);
    if (!NVMAccessControllerIsBlockSplited(&manager->nvmAccessController, addr))
    {
        physical_block_t block;
        struct BlockInfo info;

        block = nvm_addr_to_block(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr),
                                  manager->dataStartOffset);
        NVMAccessControllerTrim(&manager->nvmAccessController, addr, &shouldModifyNVMData);
        if (shouldModifyNVMData)
        {
            BlockUnmapTableGet(&manager->blockUnmapTable, block, &info);
            info.busy = 0;
            BlockUnmapTableSet(&manager->blockUnmapTable, block, &info);
        }
    }
    else
    {
        physical_page_t page;
        struct PageInfo info;

        page = nvm_addr_to_page(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr),
                                manager->dataStartOffset);
        NVMAccessControllerTrim(&manager->nvmAccessController, addr, &shouldModifyNVMData);
        if (shouldModifyNVMData)
        {
            PageUnmapTableGet(&manager->pageUnmapTable, page, &info);
            info.busy = 0;
            PageUnmapTableSet(&manager->pageUnmapTable, page, &info);
        }
    }

    NVMAccessControllerUniqueUnlock(&manager->nvmAccessController, addr);
}

void MapInfoManagerSplit(struct MapInfoManager * manager, logic_addr_t addr, struct BlockWearTable * blockWearTable,
                         struct PageWearTable * pageWearTable)
{
    physical_block_t block;
    UINT32 blockWearCount;
    struct BlockInfo info;

    NVMAccessControllerBlockUniqueLock(&manager->nvmAccessController, addr);
    block = nvm_addr_to_block(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr),
                              manager->dataStartOffset);
    blockWearCount = BlockWearTableGet(blockWearTable, block);
    BlockUnmapTableGet(&manager->blockUnmapTable, block, &info);
    PageUnmapTableBatchFormat(&manager->pageUnmapTable, block, info.busy);
    PageWearTableBatchSet(pageWearTable, block_to_page(block), blockWearCount);
    NVMAccessControllerSplit(&manager->nvmAccessController, addr);
    NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, addr);
}

static inline UINT32 CaculateBlockAverageWearCount(struct PageWearTable * pageWearTable, physical_page_t startPage)
{
    UINT32 * wearCounts;
    UINT64 sum = 0;
    int i;

    wearCounts = kmalloc(sizeof(UINT32) * 512, GFP_KERNEL);
    PageWearTableBatchGet(pageWearTable, startPage, wearCounts);
    for (i = 0; i < 512; ++i)
    {
        sum += wearCounts[i];
    }
    kfree(wearCounts);
    return sum / 512;
}

void MapInfoManagerMerge(struct MapInfoManager * manager, logic_addr_t addr, struct BlockWearTable * blockWearTable,
                         struct PageWearTable * pageWearTable)
{
    struct BlockInfo info;
    physical_block_t block;

    NVMAccessControllerBlockUniqueLock(&manager->nvmAccessController, addr);
    block = nvm_addr_to_block(NVMAccessControllerLogicAddrTranslate(&manager->nvmAccessController, addr),
                              manager->dataStartOffset);
    BlockWearTableSet(blockWearTable, block, CaculateBlockAverageWearCount(pageWearTable, block_to_page(block)));
    BlockUnmapTableGet(&manager->blockUnmapTable, block, &info);
    info.fineGrain = 0;
    BlockUnmapTableSet(&manager->blockUnmapTable, block, &info);
    NVMAccessControllerBlockUniqueUnlock(&manager->nvmAccessController, addr);
}

int MapInfoManagerIsBlockSplited(struct MapInfoManager * manager, logic_addr_t addr)
{
    return NVMAccessControllerIsBlockSplited(&manager->nvmAccessController, addr);
}