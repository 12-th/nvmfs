#include "BlockSwapTransactionLogArea.h"
#include "NVMOperations.h"
#include "WearLeveler.h"
#include "common.h"

void BlockSwapTransactionLogAreaFormat(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex)
{
    NVMemset(addr, 0, sizeof(struct NVMBlockSwapTransactionLogArea));
    pArea->addr = addr;
    pArea->startIndex = startIndex;
}

void BlockSwapTransactionLogAreaInit(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex)
{
    pArea->addr = addr;
    pArea->startIndex = startIndex;
}

static inline UINT64 StepOfBlockSwapTransactionInfo(struct BlockSwapTransactionInfo * info)
{
    if (info->flags.step4)
        return BLOCK_SWAP_STEP4;
    else if (info->flags.step3)
        return BLOCK_SWAP_STEP3;
    else if (info->flags.step2)
        return BLOCK_SWAP_STEP2;
    else
        return BLOCK_SWAP_STEP1;
}

void BlockSwapTransactionLogAreaUninit(struct BlockSwapTransactionLogArea * pArea)
{
}

static inline void UpdateLogAreaIndex(struct BlockSwapTransactionLogArea * pArea)
{
    pArea->startIndex++;
    if (pArea->startIndex >= BLOCK_SWAP_TRANSACTION_INFO_NUM_PER_AREA)
    {
        pArea = 0;
    }
}

void BlockSwapTransactionInit(struct BlockSwapTransaction * pTran, struct BlockSwapTransactionLogArea * pArea)
{
    pTran->pArea = pArea;
    pTran->transactionInfoAddr = pArea->addr + sizeof(struct BlockSwapTransactionInfo) * pArea->startIndex;
    pTran->index = pArea->startIndex;
    UpdateLogAreaIndex(pArea);
}

void BlockSwapTransactionUninit(struct BlockSwapTransaction * pTran)
{
}

static void BlockSwapTransactionCommitStep1(struct BlockSwapTransaction * pTran, physical_block_t oldBlock,
                                            physical_block_t newBlock, nvm_addr_t swapBlockAddr,
                                            struct BlockInfo * oldBlockInfo, struct BlockInfo * newBlockInfo,
                                            UINT64 swapType)
{
    struct BlockSwapTransactionInfoFlags flags = {
        .validFlag = 1, .step1 = 1, .step2 = 0, .step3 = 0, .step4 = 0, .swapType = swapType};
    struct BlockSwapTransactionInfo info = {.oldBlock = oldBlock,
                                            .newBlock = newBlock,
                                            .swapBlockAddr = swapBlockAddr,
                                            .oldBlockInfo = *oldBlockInfo,
                                            .newBlockInfo = *newBlockInfo,
                                            .flags = flags};
    //这里的数据应该先写下去，clflush以后再写flags
    NVMWrite(pTran->transactionInfoAddr, sizeof(info), &info);
}
static void BlockSwapTransactionCommitStepx(struct BlockSwapTransaction * pTran, UINT64 step, UINT64 swapType)
{
    struct BlockSwapTransactionInfoFlags flags = {
        .validFlag = 1, .step1 = 1, .step2 = 0, .step3 = 0, .step4 = 0, .swapType = swapType};

    switch (step)
    {
    case BLOCK_SWAP_STEP4:
        flags.step4 = 1;
    case BLOCK_SWAP_STEP3:
        flags.step3 = 1;
    case BLOCK_SWAP_STEP2:
        flags.step2 = 1;
    }
    NVMWrite(pTran->transactionInfoAddr + offsetof(struct BlockSwapTransactionInfo, flags), sizeof(flags), &flags);
}

void DoBlockCompleteSwapTransaction(struct BlockSwapTransaction * pTran, struct MapInfoManager * manager,
                                    struct BlockMapInfo * oldBlockInfo, struct BlockMapInfo * newBlockInfo,
                                    nvm_addr_t swapBlockAddr, nvm_addr_t dataStartOffset)
{
    nvm_addr_t oldBlockAddr, newBlockAddr;

    oldBlockAddr = block_to_nvm_addr(oldBlockInfo->physBlock, dataStartOffset);
    newBlockAddr = block_to_nvm_addr(newBlockInfo->physBlock, dataStartOffset);
    BlockSwapTransactionCommitStep1(pTran, oldBlockInfo->physBlock, newBlockInfo->physBlock, swapBlockAddr,
                                    &oldBlockInfo->blockInfo, &newBlockInfo->blockInfo, BLOCK_SWAP_COMPLETE);
    NVMemcpy(swapBlockAddr, oldBlockAddr, SIZE_2M);

    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP2, BLOCK_SWAP_COMPLETE);
    NVMemcpy(oldBlockAddr, newBlockAddr, SIZE_2M);

    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP3, BLOCK_SWAP_COMPLETE);
    NVMemcpy(newBlockAddr, swapBlockAddr, SIZE_2M);
    MapInfoManagerSwapBlock(manager, oldBlockInfo, newBlockInfo);
    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP4, BLOCK_SWAP_COMPLETE);
}

void DoBlockQuickSwapTransaction(struct BlockSwapTransaction * pTran, struct MapInfoManager * manager,
                                 struct BlockMapInfo * oldBlockInfo, struct BlockMapInfo * newBlockInfo,
                                 nvm_addr_t dataStartOffset)
{
    BlockSwapTransactionCommitStep1(pTran, oldBlockInfo->physBlock, newBlockInfo->physBlock, 0,
                                    &oldBlockInfo->blockInfo, &newBlockInfo->blockInfo, BLOCK_SWAP_QUICK);
    NVMemcpy(block_to_nvm_addr(newBlockInfo->physBlock, dataStartOffset),
             block_to_nvm_addr(oldBlockInfo->physBlock, dataStartOffset), SIZE_2M);
    MapInfoManagerSwapBlock(manager, oldBlockInfo, newBlockInfo);
    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP2, BLOCK_SWAP_QUICK);
}

void DoBlockSwapTransaction(struct BlockSwapTransaction * pTran, struct MapInfoManager * manager,
                            struct BlockMapInfo * oldBlockInfo, struct BlockMapInfo * newBlockInfo,
                            struct SwapTable * pSwapTable, nvm_addr_t dataStartOffset)
{
    if (newBlockInfo->blockInfo.busy)
    {
        DoBlockCompleteSwapTransaction(pTran, manager, oldBlockInfo, newBlockInfo, GetSwapBlock(pSwapTable),
                                       dataStartOffset);
    }
    else
    {
        DoBlockQuickSwapTransaction(pTran, manager, oldBlockInfo, newBlockInfo, dataStartOffset);
    }
}

// static void BlockCompleteSwapTransactionRecovery(struct BlockSwapTransaction * pTran, struct MapInfoManager *
// manager,
//                                                  nvm_addr_t dataStartOffset, struct BlockSwapTransactionInfo * info,
//                                                  struct BlockSwapTransactionLogArea * pArea)
// {
//     UINT64 step;

//     step = StepOfBlockSwapTransactionInfo(info);
//     switch (step)
//     {
//     case BLOCK_SWAP_STEP1:
//         NVMemcpy(info->swapBlockAddr, block_to_nvm_addr(info->oldBlock, dataStartOffset), SIZE_2M);
//         BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP2, BLOCK_SWAP_COMPLETE);

//     case BLOCK_SWAP_STEP2:
//         NVMemcpy(block_to_nvm_addr(info->oldBlock, dataStartOffset), block_to_nvm_addr(info->newBlock,
//         dataStartOffset),
//                  SIZE_2M);
//         BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP3, BLOCK_SWAP_COMPLETE);
//     case BLOCK_SWAP_STEP3:
//         NVMemcpy(block_to_nvm_addr(info->newBlock, dataStartOffset), info->swapBlockAddr, SIZE_2M);
//         BlockUnmapTableSet(pBlockUnmapTable, info->oldBlock, &info->newBlockInfo);
//         BlockUnmapTableSet(pBlockUnmapTable, info->newBlock, &info->oldBlockInfo);
//         BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP4, BLOCK_SWAP_COMPLETE);
//         UpdateLogAreaIndex(pArea);
//     case BLOCK_SWAP_STEP4:
//         break;
//     }
// }

// static void BlockQuickSwapTransactionRecovery(struct BlockSwapTransaction * pTran,
//                                               struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t dataStartOffset,
//                                               struct BlockSwapTransactionInfo * info,
//                                               struct BlockSwapTransactionLogArea * pArea)
// {
//     UINT64 step;

//     step = StepOfBlockSwapTransactionInfo(info);
//     switch (step)
//     {
//     case BLOCK_SWAP_STEP1:
//         NVMemcpy(block_to_nvm_addr(info->newBlock, dataStartOffset), block_to_nvm_addr(info->oldBlock,
//         dataStartOffset),
//                  SIZE_2M);
//         BlockUnmapTableSet(pBlockUnmapTable, info->oldBlock, &info->newBlockInfo);
//         BlockUnmapTableSet(pBlockUnmapTable, info->newBlock, &info->oldBlockInfo);
//         BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP2, BLOCK_SWAP_QUICK);
//     case BLOCK_SWAP_STEP2:
//         break;
//     }
// }

// void BlockSwapTransactionLogAreaRecovery(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr,
//                                          struct WearLeveler * wl)
// {
//     int i;
//     nvm_addr_t iter;
//     struct BlockSwapTransactionInfo info;

//     pArea->addr = addr;
//     iter = addr;

//     for (i = 0; i < BLOCK_SWAP_TRANSACTION_INFO_NUM_PER_AREA; ++i)
//     {
//         struct BlockSwapTransaction tran;

//         tran.transactionInfoAddr = i * sizeof(struct BlockSwapTransactionInfo);
//         tran.pArea = pArea;
//         tran.index = i;

//         NVMRead(iter, sizeof(info), &info);
//         iter += sizeof(struct BlockSwapTransactionInfo);

//         if (info.flags.validFlag)
//         {
//             if (info.flags.swapType == BLOCK_SWAP_COMPLETE)
//             {
//                 BlockCompleteSwapTransactionRecovery(&tran, wl, &info, pArea);
//             }
//             else
//             {
//                 BlockQuickSwapTransactionRecovery(&tran, wl, &info, pArea);
//             }
//         }
//     }
// }