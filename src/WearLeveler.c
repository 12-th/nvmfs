#include "Config.h"
#include "NVMOperations.h"
#include "WearLeveler.h"
#include "common.h"

nvm_addr_t LogicAddressTranslate(struct WearLeveler * wl, logic_addr_t addr)
{
    return MapAddressQuery(&wl->mapTable, addr);
}

void NVMBlockWearCountIncrease(struct WearLeveler * wl, logic_addr_t logicAddr, UINT32 delta)
{
    UINT32 oldBlockOldWearCount, oldBlockNewWearCount;
    UINT32 newBlockOldWearCount;
    nvm_addr_t swapBlockAddr;
    physical_block_t newBlock, oldBlock;
    struct BlockInfo newBlockInfo, oldBlockInfo;
    struct BlockSwapTransaction tran;

    oldBlock = nvm_addr_to_block(MapAddressQuery(&wl->mapTable, logicAddr), &wl->layouter);
    oldBlockOldWearCount = GetBlockWearCount(&wl->blockWearTable, oldBlock);
    oldBlockNewWearCount = oldBlockOldWearCount + delta;
    UpdateBlockWearCount(&wl->blockWearTable, oldBlock, oldBlockNewWearCount);
    if (!IsCrossWearCountThreshold(oldBlockOldWearCount, oldBlockNewWearCount))
    {
        return;
    }

    AvailBlockTableSwapPrepare(&wl->availBlockTable, oldBlock, &newBlock);
    BlockUnmapTableGet(&wl->blockUnmapTable, newBlock, &newBlockInfo);
    BlockUnmapTableGet(&wl->blockUnmapTable, oldBlock, &oldBlockInfo);
    swapBlockAddr = GetSwapBlock(&wl->swapTable);

    BlockSwapTransactionInit(&tran, &wl->blockSwapTransactionLogArea);
    DoBlockSwapTransaction(&tran, wl, oldBlock, newBlock, swapBlockAddr, &oldBlockInfo, &newBlockInfo);
    BlockSwapTransactionUninit(&tran);

    newBlockOldWearCount = GetBlockWearCount(&wl->blockWearTable, newBlock);
    AvailBlockTableSwapEnd(&wl->availBlockTable, oldBlock, oldBlockNewWearCount / STEP_WEAR_COUNT, newBlock,
                           (newBlockOldWearCount + 1) / STEP_WEAR_COUNT);
    UpdateBlockWearCount(&wl->blockWearTable, newBlock, newBlockOldWearCount + 1);
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
    SwapTableInit(&wl->swapTable, SwapTableMetadataAddrQuery(&wl->layouter));
    BlockSwapTransactionLogAreaInit(&wl->blockSwapTransactionLogArea,
                                    BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
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
    SwapTableFormat(&wl->swapTable, SwapTableBlockNumQuery(&wl->layouter), 0, SwapTableMetadataAddrQuery(&wl->layouter),
                    SwapTableAddrQuery(&wl->layouter));
    BlockSwapTransactionLogAreaFormat(&wl->blockSwapTransactionLogArea,
                                      BlockSwapTransactionLogAreaAddrQuery(&wl->layouter), 0);
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