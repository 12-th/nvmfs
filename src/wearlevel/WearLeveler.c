#include "Config.h"
#include "NVMOperations.h"
#include "WearCountCaculate.h"
#include "WearLeveler.h"
#include "common.h"
#include <linux/slab.h>

static inline void NVMAccessLockForSwap(struct NVMAccessController * acl, logic_addr_t addr1, logic_addr_t addr2)
{
    if (addr1 < addr2)
    {
        NVMAccessControllerUniqueLock(acl, addr1);
        NVMAccessControllerUniqueLock(acl, addr2);
    }
    else
    {
        NVMAccessControllerUniqueLock(acl, addr2);
        NVMAccessControllerUniqueLock(acl, addr1);
    }
}

static inline void NVMAccessUnlockForSwap(struct NVMAccessController * acl, logic_addr_t addr1, logic_addr_t addr2)
{
    if (addr1 < addr2)
    {
        NVMAccessControllerUniqueUnlock(acl, addr2);
        NVMAccessControllerUniqueUnlock(acl, addr1);
    }
    else
    {
        NVMAccessControllerUniqueUnlock(acl, addr1);
        NVMAccessControllerUniqueUnlock(acl, addr2);
    }
}

static void NVMBlockWearCountIncrease(struct WearLeveler * wl, logic_addr_t logicAddr, UINT32 delta)
{
    UINT32 oldBlockOldWearCount, oldBlockNewWearCount;
    UINT32 newBlockOldWearCount;
    struct BlockMapInfo oldBlockInfo, newBlockInfo;
    physical_block_t newBlock;
    struct BlockSwapTransaction tran;

    BlockWearCountIncreasePrepare(&wl->mapInfoManager, logicAddr);
    BlockMapInfoBuildFromLogicalBlock(&oldBlockInfo, &wl->mapInfoManager, logical_addr_to_block(logicAddr));
    oldBlockOldWearCount = BlockWearTableGet(&wl->blockWearTable, logical_addr_to_block(logicAddr));
    oldBlockNewWearCount = oldBlockOldWearCount + delta;
    if (!IsCrossWearCountThreshold(oldBlockOldWearCount, oldBlockNewWearCount))
    {
        BlockWearTableSet(&wl->blockWearTable, oldBlockInfo.physBlock, oldBlockNewWearCount);
        BlockWearCountIncreaseEnd(&wl->mapInfoManager, logicAddr);
        return;
    }
    BlockWearCountIncreaseEnd(&wl->mapInfoManager, logicAddr);

    oldBlockNewWearCount++;
    BlockWearTableSet(&wl->blockWearTable, oldBlockInfo.physBlock, oldBlockNewWearCount);
    AvailBlockTableSwapPrepare(&wl->availBlockTable, oldBlockInfo.physBlock, &newBlock);
    BlockSwapPrepare(&wl->mapInfoManager, &oldBlockInfo, &newBlockInfo, oldBlockInfo.physBlock, newBlock);
    newBlockOldWearCount = BlockWearTableGet(&wl->blockWearTable, newBlockInfo.physBlock);

    BlockSwapTransactionInit(&tran, &wl->blockSwapTransactionLogArea);
    DoBlockSwapTransaction(&tran, &wl->mapInfoManager, &oldBlockInfo, &newBlockInfo, &wl->swapTable,
                           DataStartAddrQuery(&wl->layouter));
    BlockSwapTransactionUninit(&tran);

    BlockSwapEnd(&wl->mapInfoManager, &oldBlockInfo, &newBlockInfo);
    AvailBlockTableSwapEnd(&wl->availBlockTable, oldBlockInfo.physBlock, oldBlockNewWearCount / STEP_WEAR_COUNT,
                           newBlockInfo.physBlock, (newBlockOldWearCount + 1) / STEP_WEAR_COUNT);
    BlockWearTableSet(&wl->blockWearTable, newBlockInfo.physBlock, newBlockOldWearCount + 1);
}

static void NVMPageWearCountIncrease(struct WearLeveler * wl, logic_addr_t addr, UINT32 delta)
{
    UINT32 oldPageOldWearCount, newPageOldWearCount, oldPageNewWearCount;
    struct PageMapInfo oldPageInfo, newPageInfo;
    UINT32 wearCountThreshold, blockWearCount;
    int shouldSwapBlock;
    struct PageSwapTransaction tran;

    PageWearCountIncreasePrepare(&wl->mapInfoManager, addr);
    PageMapInfoBuildFromLogicalPage(&oldPageInfo, &wl->mapInfoManager, logical_addr_to_page(addr));
    oldPageOldWearCount = PageWearTableGet(&wl->pageWearTable, oldPageInfo.physPage);
    oldPageNewWearCount = oldPageOldWearCount + delta;
    if (!IsCrossWearCountThreshold(oldPageOldWearCount, oldPageNewWearCount))
    {
        PageWearCountSet(&wl->pageWearTable, oldPageInfo.physPage, oldPageNewWearCount);
        PageWearCountIncreaseEnd(&wl->mapInfoManager, addr);
        return;
    }

    PageWearCountSet(&wl->pageWearTable, oldPageInfo.physPage, oldPageNewWearCount + 1);
    blockWearCount = BlockWearTableGet(&wl->blockWearTable, page_to_block(oldPageInfo.physPage));
    wearCountThreshold = NextWearCountThreshold(blockWearCount);
    AvailPageTableSwapPrepare(&wl->availPageTable, &wl->pageWearTable, oldPageInfo.physPage, &newPageInfo.physPage,
                              wearCountThreshold, &shouldSwapBlock);
    if (shouldSwapBlock)
    {
        PageWearCountIncreaseEnd(&wl->mapInfoManager, addr);
        NVMBlockWearCountIncrease(wl, addr, wearCountThreshold - blockWearCount);
        return;
    }

    PageWearCountIncreaseToSwapPrepare(&wl->mapInfoManager, &oldPageInfo, &newPageInfo, oldPageInfo.physPage,
                                       newPageInfo.physPage);
    PageSwapTransactionInit(&tran, &wl->pageSwapTransactionLogArea);
    DoPageSwapTransaction(&tran, &oldPageInfo, &newPageInfo, &wl->mapInfoManager, &wl->swapTable,
                          DataStartAddrQuery(&wl->layouter));
    PageSwapTransactionUninit(&tran);
    PageSwapEnd(&wl->mapInfoManager, &oldPageInfo, &newPageInfo);

    newPageOldWearCount = PageWearTableGet(&wl->pageWearTable, newPageInfo.physPage);
    AvailPageTableSwapEnd(&wl->availPageTable, newPageInfo.physPage, newPageOldWearCount + 1);
    PageWearCountSet(&wl->pageWearTable, newPageInfo.physPage, newPageOldWearCount + 1);
}

void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize)
{
    UINT64 blockNum, pageNum;

    LayouterInit(&wl->layouter, nvmSizeBits, reserveSize);
    blockNum = BlockNumQuery(&wl->layouter);
    pageNum = PageNumQuery(&wl->layouter);

    BlockWearTableFormat(&wl->blockWearTable, BlockWearTableAddrQuery(&wl->layouter), blockNum);
    PageWearTableFormat(&wl->pageWearTable, PageWearTableAddrQuery(&wl->layouter), pageNum);
    MapInfoManagerFormat(&wl->mapInfoManager, &wl->layouter);

    AvailBlockTableRebuild(&wl->availBlockTable, &wl->blockWearTable, blockNum);
    AvailPageTableInit(&wl->availPageTable, pageNum);
    SwapTableFormat(&wl->swapTable, SwapTableMetadataAddrQuery(&wl->layouter), SwapTableBlocksNumQuery(&wl->layouter),
                    SwapTablePagesNumQuery(&wl->layouter), SwapTableBlocksAddrQuery(&wl->layouter),
                    SwapTablePagesAddrQuery(&wl->layouter));
    BlockSwapTransactionLogAreaFormat(&wl->blockSwapTransactionLogArea,
                                      BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
    PageSwapTransactionLogAreaFormat(&wl->pageSwapTransactionLogArea,
                                     PageSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
}

void WearLevelerUninit(struct WearLeveler * wl)
{
    BlockSwapTransactionLogAreaUninit(&wl->blockSwapTransactionLogArea);
    SwapTableUninit(&wl->swapTable);
    AvailBlockTableUninit(&wl->availBlockTable);
    PageWearTableUninit(&wl->pageWearTable);
    MapInfoManagerUninit(&wl->mapInfoManager);
    BlockWearTableUninit(&wl->blockWearTable);
}

static void WearLevelerRecovery(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize)
{
    nvm_addr_t dataStartOffset;

    LayouterInit(&wl->layouter, nvmSizeBits, reserveSize);
    dataStartOffset = DataStartAddrQuery(&wl->layouter);
    MapInfoManagerRecoveryBegin(&wl->mapInfoManager, &wl->layouter);
    BlockSwapTransactionLogAreaRecovery(&wl->blockSwapTransactionLogArea, &wl->mapInfoManager,
                                        BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), dataStartOffset);
    PageSwapTransactionLogAreaRecovery(&wl->pageSwapTransactionLogArea, &wl->mapInfoManager,
                                       PageSwapTransactionLogAreaAddrQuery(&wl->layouter), dataStartOffset);
    MapInfoManagerRecoveryEnd(&wl->mapInfoManager);
}

void WearLevelerInit(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 reserveSize)
{
    UINT64 blockNum, pageNum;

    WearLevelerRecovery(wl, nvmSizeBits, reserveSize);

    LayouterInit(&wl->layouter, nvmSizeBits, reserveSize);
    blockNum = BlockNumQuery(&wl->layouter);
    pageNum = PageNumQuery(&wl->layouter);

    BlockWearTableInit(&wl->blockWearTable, BlockWearTableAddrQuery(&wl->layouter), blockNum);
    PageWearTableInit(&wl->pageWearTable, PageUnmapTableAddrQuery(&wl->layouter), pageNum);
    MapInfoManagerInit(&wl->mapInfoManager, &wl->layouter);

    AvailBlockTableRebuild(&wl->availBlockTable, &wl->blockWearTable, blockNum);
    AvailPageTableInit(&wl->availPageTable, pageNum);
    SwapTableInit(&wl->swapTable, SwapTableMetadataAddrQuery(&wl->layouter));
    BlockSwapTransactionLogAreaInit(&wl->blockSwapTransactionLogArea,
                                    BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
    PageSwapTransactionLogAreaInit(&wl->pageSwapTransactionLogArea, PageSwapTransactionLogAreaAddrQuery(&wl->layouter),
                                   0);
}

void NVMBlockSplit(struct WearLeveler * wl, logic_addr_t addr)
{
    MapInfoManagerSplit(&wl->mapInfoManager, addr, &wl->blockWearTable, &wl->pageWearTable);
}

void NVMPagesMerge(struct WearLeveler * wl, logic_addr_t addr)
{
    MapInfoManagerMerge(&wl->mapInfoManager, addr, &wl->blockWearTable, &wl->pageWearTable);
}

int WearLevelerRead(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size)
{
    return MapInfoManagerRead(&wl->mapInfoManager, addr, buffer, size);
}

UINT32 WearLevelerWrite(struct WearLeveler * wl, logic_addr_t addr, void * buffer, UINT32 size,
                        UINT32 increasedWearCount)
{
    int writeSize;

    writeSize = MapInfoManagerWrite(&wl->mapInfoManager, addr, buffer, size);
    if (increasedWearCount)
    {
        if (!MapInfoManagerIsBlockSplited(&wl->mapInfoManager, addr))
        {
            NVMBlockWearCountIncrease(wl, addr, increasedWearCount);
        }
        else
        {
            NVMPageWearCountIncrease(wl, addr, increasedWearCount);
        }
    }
    return writeSize;
}

UINT32 WearLevelerMemset(struct WearLeveler * wl, logic_addr_t addr, UINT32 size, int value, UINT32 increasedWearCount)
{
    int writeSize;

    writeSize = MapInfoManagerMemset(&wl->mapInfoManager, addr, value, size);
    if (increasedWearCount)
    {
        if (!MapInfoManagerIsBlockSplited(&wl->mapInfoManager, addr))
        {
            NVMBlockWearCountIncrease(wl, addr, increasedWearCount);
        }
        else
        {
            NVMPageWearCountIncrease(wl, addr, increasedWearCount);
        }
    }
    return writeSize;
}

UINT32 WearLevelerMemcpy(struct WearLeveler * wl, logic_addr_t srcAddr, logic_addr_t dstAddr, UINT32 size,
                         UINT32 increasedWearCount)
{
    int writeSize = 0;
    int isDstFullWrite;

    writeSize = MapInfoManagerMemcpy(&wl->mapInfoManager, srcAddr, dstAddr, size, &isDstFullWrite);
    if (increasedWearCount && isDstFullWrite)
    {
        if (!MapInfoManagerIsBlockSplited(&wl->mapInfoManager, dstAddr))
        {
            NVMBlockWearCountIncrease(wl, dstAddr, increasedWearCount);
        }
        else
        {
            NVMPageWearCountIncrease(wl, dstAddr, increasedWearCount);
        }
    }
    return writeSize;
}

void WearLevelerTrim(struct WearLeveler * wl, logic_addr_t addr)
{
    MapInfoManagerTrim(&wl->mapInfoManager, addr);
}