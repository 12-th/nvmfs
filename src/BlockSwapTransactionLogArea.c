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
                                            struct BlockInfo * oldBlockInfo, struct BlockInfo * newBlockInfo)
{
    struct BlockSwapTransactionInfoFlags flags = {.validFlag = 1, .step1 = 1, .step2 = 0, .step3 = 0, .step4 = 0};
    struct BlockSwapTransactionInfo info = {.oldBlock = oldBlock,
                                            .newBlock = newBlock,
                                            .swapBlockAddr = swapBlockAddr,
                                            .oldBlockInfo = *oldBlockInfo,
                                            .newBlockInfo = *newBlockInfo,
                                            .flags = flags};
    //这里的数据应该先写下去，clflush以后再写flags
    NVMWrite(pTran->transactionInfoAddr, sizeof(info), &info);
}
static void BlockSwapTransactionCommitStepx(struct BlockSwapTransaction * pTran, UINT64 step)
{
    struct BlockSwapTransactionInfoFlags flags = {.validFlag = 1, .step1 = 1, .step2 = 0, .step3 = 0, .step4 = 0};

    switch (step)
    {
    case BLOCK_SWAP_STEP2:
        flags.step2 = 1;
    case BLOCK_SWAP_STEP3:
        flags.step3 = 1;
    case BLOCK_SWAP_STEP4:
        flags.step4 = 1;
    }
    NVMWrite(pTran->transactionInfoAddr + offsetof(struct BlockSwapTransactionInfo, flags), sizeof(flags), &flags);
}

void DoBlockSwapTransaction(struct BlockSwapTransaction * pTran, struct WearLeveler * wl, physical_block_t oldBlock,
                            physical_block_t newBlock, nvm_addr_t swapBlockAddr, struct BlockInfo * oldBlockInfo,
                            struct BlockInfo * newBlockInfo)
{
    BlockSwapTransactionCommitStep1(pTran, oldBlock, newBlock, swapBlockAddr, oldBlockInfo, newBlockInfo);
    NVMemcpy(swapBlockAddr, block_to_nvm_addr(oldBlock, &wl->layouter), SIZE_2M);

    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP2);
    NVMemcpy(block_to_nvm_addr(oldBlock, &wl->layouter), block_to_nvm_addr(newBlock, &wl->layouter), SIZE_2M);

    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP3);
    NVMemcpy(block_to_nvm_addr(newBlock, &wl->layouter), swapBlockAddr, SIZE_2M);
    BlockUnmapTableSet(&wl->blockUnmapTable, oldBlock, newBlockInfo);
    BlockUnmapTableSet(&wl->blockUnmapTable, newBlock, oldBlockInfo);
    BlockMapModify(&wl->mapTable, oldBlockInfo->unmapBlock, newBlock);
    BlockMapModify(&wl->mapTable, newBlockInfo->unmapBlock, oldBlock);

    BlockSwapTransactionCommitStepx(pTran, BLOCK_SWAP_STEP4);
}

void BlockSwapTransactionLogAreaRecovery(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr,
                                         struct WearLeveler * wl)
{
    int i;
    nvm_addr_t iter;
    struct BlockSwapTransactionInfo info;

    pArea->addr = addr;
    iter = addr;

    for (i = 0; i < BLOCK_SWAP_TRANSACTION_INFO_NUM_PER_AREA; ++i)
    {
        struct BlockSwapTransaction tran;

        tran.transactionInfoAddr = i * sizeof(struct BlockSwapTransactionInfo);
        tran.pArea = pArea;
        tran.index = i;

        NVMRead(iter, sizeof(info), &info);
        iter += sizeof(struct BlockSwapTransactionInfo);

        if (info.flags.validFlag)
        {
            UINT64 step;

            step = StepOfBlockSwapTransactionInfo(&info);
            switch (step)
            {
            case BLOCK_SWAP_STEP1:
                NVMemcpy(info.swapBlockAddr, block_to_nvm_addr(info.oldBlock, &wl->layouter), SIZE_2M);
                BlockSwapTransactionCommitStepx(&tran, BLOCK_SWAP_STEP2);

            case BLOCK_SWAP_STEP2:
                NVMemcpy(block_to_nvm_addr(info.oldBlock, &wl->layouter),
                         block_to_nvm_addr(info.newBlock, &wl->layouter), SIZE_2M);
                BlockSwapTransactionCommitStepx(&tran, BLOCK_SWAP_STEP3);
            case BLOCK_SWAP_STEP3:
                NVMemcpy(block_to_nvm_addr(info.newBlock, &wl->layouter), info.swapBlockAddr, SIZE_2M);
                BlockUnmapTableSet(&wl->blockUnmapTable, info.oldBlock, &info.newBlockInfo);
                BlockUnmapTableSet(&wl->blockUnmapTable, info.newBlock, &info.oldBlockInfo);
                BlockMapModify(&wl->mapTable, info.oldBlockInfo.unmapBlock, info.newBlock);
                BlockMapModify(&wl->mapTable, info.newBlockInfo.unmapBlock, info.oldBlock);
                BlockSwapTransactionCommitStepx(&tran, BLOCK_SWAP_STEP4);
                UpdateLogAreaIndex(pArea);
            case BLOCK_SWAP_STEP4:
                break;
            }
        }
    }
}