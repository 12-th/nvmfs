#include "Config.h"
#include "NVMOperations.h"
#include "WearCountCaculate.h"
#include "WearLeveler.h"
#include "common.h"
#include <linux/slab.h>

nvm_addr_t LogicAddressTranslate(struct WearLeveler * wl, logic_addr_t addr)
{
    return MapAddressQuery(&wl->mapTable, addr);
}

void NVMBlockWearCountIncrease(struct WearLeveler * wl, logic_addr_t logicAddr, UINT32 delta)
{
    UINT32 oldBlockOldWearCount, oldBlockNewWearCount;
    UINT32 newBlockOldWearCount;
    physical_block_t newBlock, oldBlock;
    struct BlockInfo newBlockInfo, oldBlockInfo;
    struct BlockSwapTransaction tran;

    oldBlock = nvm_addr_to_block(MapAddressQuery(&wl->mapTable, logicAddr), &wl->layouter);
    oldBlockOldWearCount = BlockWearTableGet(&wl->blockWearTable, oldBlock);
    oldBlockNewWearCount = oldBlockOldWearCount + delta;
    if (!IsCrossWearCountThreshold(oldBlockOldWearCount, oldBlockNewWearCount))
    {
        BlockWearTableSet(&wl->blockWearTable, oldBlock, oldBlockNewWearCount);
        return;
    }

    BlockWearTableSet(&wl->blockWearTable, oldBlock, oldBlockNewWearCount + 1);
    AvailBlockTableSwapPrepare(&wl->availBlockTable, oldBlock, &newBlock);
    BlockUnmapTableGet(&wl->blockUnmapTable, newBlock, &newBlockInfo);
    BlockUnmapTableGet(&wl->blockUnmapTable, oldBlock, &oldBlockInfo);

    BlockSwapTransactionInit(&tran, &wl->blockSwapTransactionLogArea);
    DoBlockSwapTransaction(&tran, wl, oldBlock, newBlock, &oldBlockInfo, &newBlockInfo);
    BlockSwapTransactionUninit(&tran);

    newBlockOldWearCount = BlockWearTableGet(&wl->blockWearTable, newBlock);
    AvailBlockTableSwapEnd(&wl->availBlockTable, oldBlock, oldBlockNewWearCount / STEP_WEAR_COUNT, newBlock,
                           (newBlockOldWearCount + 1) / STEP_WEAR_COUNT);
    BlockWearTableSet(&wl->blockWearTable, newBlock, newBlockOldWearCount + 1);
}

void NVMPageWearCountIncrease(struct WearLeveler * wl, logic_addr_t addr, UINT32 delta)
{
    UINT32 oldPageOldWearCount, newPageOldWearCount, oldPageNewWearCount;
    physical_block_t block;
    physical_page_t oldPage, newPage;
    UINT32 wearCountThreshold, blockWearCount;
    struct PageSwapTransaction tran;
    struct PageInfo oldPageInfo, newPageInfo;

    oldPage = nvm_addr_to_page(MapAddressQuery(&wl->mapTable, addr), &wl->layouter);
    block = page_to_block(oldPage);
    oldPageOldWearCount = PageWearTableGet(&wl->pageWearTable, oldPage);
    oldPageNewWearCount = oldPageOldWearCount + delta;
    if (!IsCrossWearCountThreshold(oldPageOldWearCount, oldPageNewWearCount))
    {
        PageWearCountSet(&wl->pageWearTable, oldPage, oldPageNewWearCount);
        return;
    }

    PageWearCountSet(&wl->pageWearTable, oldPage, oldPageNewWearCount + 1);
    blockWearCount = BlockWearTableGet(&wl->blockWearTable, block);
    wearCountThreshold = NextWearCountThreshold(blockWearCount);
    if (AvailPageTableSwapPrepare(&wl->availPageTable, &wl->pageWearTable, oldPage, &newPage, wearCountThreshold))
    {
        NVMBlockWearCountIncrease(wl, addr, wearCountThreshold - blockWearCount);
        return;
    }

    PageUnmapTableGet(&wl->pageUnmapTable, oldPage, &oldPageInfo);
    PageUnmapTableGet(&wl->pageUnmapTable, newPage, &newPageInfo);
    PageSwapTransactionInit(&tran, &wl->pageSwapTransactionLogArea);
    DoPageSwapTransaction(&tran, wl, oldPage, newPage, &oldPageInfo, &newPageInfo, block_to_page(block));
    PageSwapTransactionUninit(&tran);

    newPageOldWearCount = PageWearTableGet(&wl->pageWearTable, newPage);
    AvailPageTableSwapEnd(&wl->availPageTable, newPage, newPageOldWearCount + 1);
    PageWearCountSet(&wl->pageWearTable, newPage, newPageOldWearCount + 1);
}

void WearLevelerInit(struct WearLeveler * wl)
{
    UINT64 blockNum, pageNum;

    LayouterInit(&wl->layouter, NvmBitsQuery(&wl->sb));
    blockNum = BlockNumQuery(&wl->layouter);
    pageNum = PageNumQuery(&wl->layouter);

    BlockWearTableInit(&wl->blockWearTable, BlockWearTableAddrQuery(&wl->layouter), blockNum);
    BlockUnmapTableInit(&wl->blockUnmapTable, BlockUnmapTableAddrQuery(&wl->layouter), blockNum);
    PageUnmapTableInit(&wl->pageUnmapTable, &wl->blockUnmapTable, PageUnmapTableAddrQuery(&wl->layouter), pageNum);
    PageWearTableInit(&wl->pageWearTable, PageUnmapTableAddrQuery(&wl->layouter), pageNum);

    MapTableRebuild(&wl->mapTable, &wl->blockUnmapTable, &wl->pageUnmapTable, blockNum, wl->nvmBaseAddr,
                    DataStartAddrQuery(&wl->layouter));
    AvailBlockTableRebuild(&wl->availBlockTable, &wl->blockWearTable, blockNum);
    AvailPageTableInit(&wl->availPageTable, pageNum);
    SwapTableInit(&wl->swapTable, SwapTableMetadataAddrQuery(&wl->layouter));
    BlockSwapTransactionLogAreaInit(&wl->blockSwapTransactionLogArea,
                                    BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
    PageSwapTransactionLogAreaInit(&wl->pageSwapTransactionLogArea, PageSwapTransactionLogAreaAddrQuery(&wl->layouter),
                                   0);
}

void WearLevelerFormat(struct WearLeveler * wl, UINT64 nvmSizeBits, UINT64 nvmBaseAddr)
{
    UINT64 blockNum, pageNum;

    wl->nvmBaseAddr = nvmBaseAddr;
    SuperBlockFormat(&wl->sb, nvmSizeBits);
    LayouterInit(&wl->layouter, nvmSizeBits);
    blockNum = BlockNumQuery(&wl->layouter);
    pageNum = PageNumQuery(&wl->layouter);

    BlockWearTableFormat(&wl->blockWearTable, BlockWearTableAddrQuery(&wl->layouter), blockNum);
    BlockUnmapTableFormat(&wl->blockUnmapTable, BlockUnmapTableAddrQuery(&wl->layouter), blockNum);
    PageUnmapTableFormat(&wl->pageUnmapTable, &wl->blockUnmapTable, PageUnmapTableAddrQuery(&wl->layouter), pageNum,
                         PageUnmapTableSizeQuery(&wl->layouter));
    PageWearTableFormat(&wl->pageWearTable, PageWearTableAddrQuery(&wl->layouter), pageNum);

    MapTableRebuild(&wl->mapTable, &wl->blockUnmapTable, &wl->pageUnmapTable, blockNum, wl->nvmBaseAddr,
                    DataStartAddrQuery(&wl->layouter));
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
    MapTableUninit(&wl->mapTable);
    PageWearTableUninit(&wl->pageWearTable);
    PageUnmapTableUninit(&wl->pageUnmapTable);
    BlockUnmapTableUninit(&wl->blockUnmapTable);
    BlockWearTableUninit(&wl->blockWearTable);
}

static void WearLevelerRecovery(struct WearLeveler * wl)
{
}

void WearLevelerLaunch(struct WearLeveler * wl, UINT64 nvmBaseAddr)
{
    wl->nvmBaseAddr = nvmBaseAddr;
    SuperBlockInit(&wl->sb);

    switch (ShutdownFlagQuery(&wl->sb))
    {
    case SUPER_BLOCK_SHUTDOWN_FLAG_NORMAL:
        WearLevelerInit(wl);
        break;
    case SUPER_BLOCK_SHUTDWON_FLAG_ERROR:
        WearLevelerRecovery(wl);
        break;
    };
}

void NVMBlockSplit(struct WearLeveler * wl, logic_addr_t logicAddr, nvm_addr_t addr)
{
    physical_block_t block;
    struct BlockInfo info;
    UINT32 blockWearCount;

    block = nvm_addr_to_block(addr, &wl->layouter);
    blockWearCount = BlockWearTableGet(&wl->blockWearTable, block);
    BlockUnmapTableGet(&wl->blockUnmapTable, block, &info);
    PageUnmapTableBatchFormat(&wl->pageUnmapTable, block, info.busy);
    PageWearTableBatchSet(&wl->pageWearTable, block_to_page(block), blockWearCount);
    info.fineGrain = 1;
    BlockUnmapTableSet(&wl->blockUnmapTable, block, &info);
    MapTableSplitBlockMap(&wl->mapTable, logical_addr_to_block(logicAddr), block);
}

void NVMPagesMerge(struct WearLeveler * wl, nvm_addr_t addr)
{
    physical_block_t block;
    physical_page_t page;
    UINT32 * wearCounts;
    UINT64 sum = 0;
    int i;
    struct BlockInfo info;

    wearCounts = kmalloc(sizeof(UINT32) * 512, GFP_KERNEL);
    block = nvm_addr_to_block(addr, &wl->layouter);
    page = block_to_page(block);
    PageWearTableBatchGet(&wl->pageWearTable, page, wearCounts);
    BlockUnmapTableGet(&wl->blockUnmapTable, block, &info);
    for (i = 0; i < 512; ++i)
    {
        sum += wearCounts[i];
    }
    BlockWearTableSet(&wl->blockWearTable, block, sum / 512);
    info.fineGrain = 0;
    BlockUnmapTableSet(&wl->blockUnmapTable, block, &info);
}

void NVMBlockInUse(struct WearLeveler * wl, nvm_addr_t addr)
{
    physical_block_t block;
    struct BlockInfo info;

    block = nvm_addr_to_block(addr, &wl->layouter);
    BlockUnmapTableGet(&wl->blockUnmapTable, block, &info);
    info.busy = 1;
    BlockUnmapTableSet(&wl->blockUnmapTable, block, &info);
}

void NVMPageInUse(struct WearLeveler * wl, nvm_addr_t addr)
{
    physical_page_t page;
    struct PageInfo info;

    page = nvm_addr_to_page(addr, &wl->layouter);
    PageUnmapTableGet(&wl->pageUnmapTable, page, &info);
    info.busy = 1;
    PageUnmapTableSet(&wl->pageUnmapTable, page, &info);
}

void NVMBlockTrim(struct WearLeveler * wl, nvm_addr_t addr)
{
    physical_block_t block;
    struct BlockInfo info;

    block = nvm_addr_to_block(addr, &wl->layouter);
    BlockUnmapTableGet(&wl->blockUnmapTable, block, &info);
    info.busy = 0;
    BlockUnmapTableSet(&wl->blockUnmapTable, block, &info);
}

void NVMPageTrim(struct WearLeveler * wl, nvm_addr_t addr)
{
    physical_page_t page;
    struct PageInfo info;

    page = nvm_addr_to_page(addr, &wl->layouter);
    PageUnmapTableGet(&wl->pageUnmapTable, page, &info);
    info.busy = 0;
    PageUnmapTableSet(&wl->pageUnmapTable, page, &info);
}