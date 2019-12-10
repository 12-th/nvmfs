#include "Config.h"
#include "Layouter.h"
#include "MapInfoManager.h"
#include "NVMOperations.h"
#include "PageSwapTransactionLogArea.h"
#include "PageUnmapTable.h"
#include "SwapTable.h"
#include "WearLeveler.h"
#include "common.h"

void PageSwapTransactionLogAreaFormat(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex)
{
    NVMemset(addr, 0, sizeof(struct PageSwapTransactionInfo) * PAGE_SWAP_TRANSACTION_INFO_NUM_PER_AREA);
    pArea->addr = addr;
    pArea->index = startIndex;
}

void PageSwapTransactionLogAreaInit(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex)
{
    pArea->addr = addr;
    pArea->index = startIndex;
}

void PageSwapTransactionLogAreaUninit(struct PageSwapTransactionLogArea * pArea)
{
}

static void UpdatePageSwapTransactionLogAreaIndex(struct PageSwapTransactionLogArea * pArea)
{
    pArea->index++;
    if (pArea->index >= PAGE_SWAP_TRANSACTION_INFO_NUM_PER_AREA)
    {
        pArea->index = 0;
    }
}

static void PageSwapTransactionCommitStep1(struct PageSwapTransaction * pTran, physical_page_t oldPage,
                                           physical_page_t newPage, struct PageInfo * oldPageInfo,
                                           struct PageInfo * newPageInfo, UINT64 swapType)
{
    struct PageSwapTransactionInfoFlags flags = {
        .validFlag = 1, .swapType = swapType, .step1 = 1, .step2 = 0, .step3 = 0, .step4 = 0};
    struct PageSwapTransactionInfo info = {.oldPage = oldPage,
                                           .newPage = newPage,
                                           .oldPageInfo = *oldPageInfo,
                                           .newPageInfo = *newPageInfo,
                                           .flags = flags};
    NVMWrite(pTran->addr, sizeof(struct PageSwapTransactionInfo), &info);
}

static inline UINT64 StepOfPageSwapTransaction(struct PageSwapTransactionInfo * info)
{
    if (info->flags.step4)
        return PAGE_SWAP_STEP4;
    else if (info->flags.step3)
        return PAGE_SWAP_STEP3;
    else if (info->flags.step2)
        return PAGE_SWAP_STEP2;
    else
        return PAGE_SWAP_STEP1;
}

static void PageSwapTransactionCommitStepx(struct PageSwapTransaction * pTran, UINT64 step, UINT64 swapType)
{
    struct PageSwapTransactionInfoFlags flags = {.validFlag = 1, .swapType = swapType};

    switch (step)
    {
    case PAGE_SWAP_STEP4:
        flags.step4 = 1;
    case PAGE_SWAP_STEP3:
        flags.step3 = 1;
    case PAGE_SWAP_STEP2:
        flags.step2 = 1;
    default:
        break;
    }
    NVMWrite(pTran->addr + (unsigned long)offsetof(struct PageSwapTransactionInfo, flags),
             sizeof(struct PageSwapTransactionInfo), &flags);
}

void PageSwapTransactionInit(struct PageSwapTransaction * pTran, struct PageSwapTransactionLogArea * pArea)
{
    pTran->addr = pArea->addr + pArea->index * sizeof(struct PageSwapTransactionInfo);
    pTran->index = pArea->index;
    UpdatePageSwapTransactionLogAreaIndex(pArea);
}

void PageSwapTransactionUninit(struct PageSwapTransaction * pTran)
{
}

void DoPageQuickSwapTransaction(struct PageSwapTransaction * pTran, struct PageMapInfo * oldPageInfo,
                                struct PageMapInfo * newPageInfo, nvm_addr_t dataStartOffset,
                                struct MapInfoManager * manager)
{
    PageSwapTransactionCommitStep1(pTran, oldPageInfo->physPage, newPageInfo->physPage, &oldPageInfo->pageInfo,
                                   &newPageInfo->pageInfo, PAGE_SWAP_QUICK);
    NVMemcpy(page_to_nvm_addr(newPageInfo->physPage, dataStartOffset),
             page_to_nvm_addr(oldPageInfo->physPage, dataStartOffset), PAGE_SIZE);
    MapInfoManagerSwapPage(manager, oldPageInfo, newPageInfo);
    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_QUICK);
}

void PageQuickSwapTransactionRecovery(struct PageSwapTransaction * pTran, struct PageSwapTransactionInfo * info,
                                      nvm_addr_t dataStartOffset, struct MapInfoManager * manager)
{
    switch (StepOfPageSwapTransaction(info))
    {
    case PAGE_SWAP_STEP1:
        NVMemcpy(page_to_nvm_addr(info->oldPage, dataStartOffset), page_to_nvm_addr(info->newPage, dataStartOffset),
                 PAGE_SIZE);
        MapInfoManagerRecoverySwapPage(manager, info->oldPage, &info->newPageInfo, info->newPage, &info->newPageInfo);
        PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_QUICK);
    case PAGE_SWAP_STEP2:
        break;
    }
}

void DoPageCompleteSwapTransaction(struct PageSwapTransaction * pTran, struct PageMapInfo * oldPageInfo,
                                   struct PageMapInfo * newPageInfo, nvm_addr_t dataStartOffset,
                                   struct MapInfoManager * manager, nvm_addr_t swapPageAddr)
{
    PageSwapTransactionCommitStep1(pTran, oldPageInfo->physPage, newPageInfo->physPage, &oldPageInfo->pageInfo,
                                   &newPageInfo->pageInfo, PAGE_SWAP_COMPLETE);
    NVMemcpy(swapPageAddr, page_to_nvm_addr(oldPageInfo->physPage, dataStartOffset), SIZE_4K);

    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_COMPLETE);
    NVMemcpy(page_to_nvm_addr(oldPageInfo->physPage, dataStartOffset),
             page_to_nvm_addr(newPageInfo->physPage, dataStartOffset), SIZE_4K);

    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP3, PAGE_SWAP_COMPLETE);
    NVMemcpy(page_to_nvm_addr(newPageInfo->physPage, dataStartOffset), swapPageAddr, SIZE_4K);
    MapInfoManagerSwapPage(manager, oldPageInfo, newPageInfo);
    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP4, PAGE_SWAP_COMPLETE);
}

void PageCompleteSwapTransactionRecovery(struct PageSwapTransaction * pTran, struct PageSwapTransactionInfo * info,
                                         nvm_addr_t dataStartOffset, struct MapInfoManager * manager)
{
    switch (StepOfPageSwapTransaction(info))
    {
    case PAGE_SWAP_STEP1:
        NVMemcpy(info->swapPageAddr, page_to_nvm_addr(info->oldPage, dataStartOffset), SIZE_4K);
        PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_COMPLETE);

    case PAGE_SWAP_STEP2:
        NVMemcpy(page_to_nvm_addr(info->oldPage, dataStartOffset), page_to_nvm_addr(info->newPage, dataStartOffset),
                 SIZE_4K);
        PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP3, PAGE_SWAP_COMPLETE);

    case PAGE_SWAP_STEP3:
        NVMemcpy(page_to_nvm_addr(info->newPage, dataStartOffset), info->swapPageAddr, SIZE_4K);
        MapInfoManagerRecoverySwapPage(manager, info->oldPage, &info->newPageInfo, info->newPage, &info->oldPageInfo);
        PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP4, PAGE_SWAP_COMPLETE);
    case PAGE_SWAP_STEP4:
        break;
    }
}

void DoPageSwapTransaction(struct PageSwapTransaction * pTran, struct PageMapInfo * oldPageInfo,
                           struct PageMapInfo * newPageInfo, struct MapInfoManager * manager,
                           struct SwapTable * pSwapTable, nvm_addr_t dataStartOffset)
{
    if (!newPageInfo->pageInfo.busy)
    {
        DoPageQuickSwapTransaction(pTran, oldPageInfo, newPageInfo, dataStartOffset, manager);
    }
    else
    {
        DoPageCompleteSwapTransaction(pTran, oldPageInfo, newPageInfo, dataStartOffset, manager,
                                      GetSwapPage(pSwapTable));
    }
}

void PageSwapTransactionLogAreaRecovery(struct PageSwapTransactionLogArea * pArea, struct MapInfoManager * manager,
                                        nvm_addr_t areaAddr, nvm_addr_t dataStartOffset)
{
    int i;
    nvm_addr_t iter;
    struct PageSwapTransactionInfo info;

    pArea->addr = areaAddr;
    iter = areaAddr;

    for (i = 0; i < PAGE_SWAP_TRANSACTION_INFO_NUM_PER_AREA; ++i)
    {
        struct PageSwapTransaction tran;

        tran.addr = i * sizeof(struct PageSwapTransaction);
        tran.index = i;

        NVMRead(iter, sizeof(info), &info);
        iter += sizeof(struct PageSwapTransaction);

        if (info.flags.validFlag)
        {
            if (info.flags.swapType == BLOCK_SWAP_COMPLETE)
            {
                PageCompleteSwapTransactionRecovery(&tran, &info, dataStartOffset, manager);
            }
            else
            {
                PageQuickSwapTransactionRecovery(&tran, &info, dataStartOffset, manager);
            }
        }
    }
}