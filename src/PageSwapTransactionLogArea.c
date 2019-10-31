#include "Config.h"
#include "Layouter.h"
#include "MapTable.h"
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

void PageSwapTransactionLogAreaRecovery(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr)
{
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

void DoPageQuickSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                                physical_page_t newPage, struct PageInfo * oldPageInfo, struct PageInfo * newPageInfo,
                                logical_page_t basePageSeq)
{
    PageSwapTransactionCommitStep1(pTran, oldPage, newPage, oldPageInfo, newPageInfo, PAGE_SWAP_QUICK);
    NVMemcpy(page_to_nvm_addr(newPage, &wl->layouter), page_to_nvm_addr(oldPage, &wl->layouter), PAGE_SIZE);
    PageUnmapTableSet(&wl->pageUnmapTable, newPage, oldPageInfo);
    PageUnmapTableSet(&wl->pageUnmapTable, oldPage, newPageInfo);
    PageMapModify(&wl->mapTable, basePageSeq + oldPageInfo->unmapPage, newPage);
    PageMapModify(&wl->mapTable, basePageSeq + newPageInfo->unmapPage, oldPage);
    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_QUICK);
}

void DoPageCompleteSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                                   physical_page_t newPage, struct PageInfo * oldPageInfo,
                                   struct PageInfo * newPageInfo, nvm_addr_t swapPageAddr, logical_page_t basePageSeq)
{
    PageSwapTransactionCommitStep1(pTran, oldPage, newPage, oldPageInfo, newPageInfo, PAGE_SWAP_COMPLETE);
    NVMemcpy(swapPageAddr, page_to_nvm_addr(oldPage, &wl->layouter), SIZE_4K);

    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP2, PAGE_SWAP_COMPLETE);
    NVMemcpy(page_to_nvm_addr(oldPage, &wl->layouter), page_to_nvm_addr(newPage, &wl->layouter), SIZE_4K);

    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP3, PAGE_SWAP_COMPLETE);
    NVMemcpy(page_to_nvm_addr(newPage, &wl->layouter), swapPageAddr, SIZE_4K);
    PageUnmapTableSet(&wl->pageUnmapTable, newPage, oldPageInfo);
    PageUnmapTableSet(&wl->pageUnmapTable, oldPage, newPageInfo);
    PageMapModify(&wl->mapTable, basePageSeq + oldPageInfo->unmapPage, newPage);
    PageMapModify(&wl->mapTable, basePageSeq + newPageInfo->unmapPage, oldPage);

    PageSwapTransactionCommitStepx(pTran, PAGE_SWAP_STEP4, PAGE_SWAP_COMPLETE);
}

void DoPageSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                           physical_page_t newPage, struct PageInfo * oldPageInfo, struct PageInfo * newPageInfo,
                           logical_page_t basePageSeq)
{
    if (!newPageInfo->busy)
    {
        DoPageQuickSwapTransaction(pTran, wl, oldPage, newPage, oldPageInfo, newPageInfo, basePageSeq);
    }
    else
    {
        nvm_addr_t swapPageAddr;

        swapPageAddr = GetSwapPage(&wl->swapTable);
        DoPageCompleteSwapTransaction(pTran, wl, oldPage, newPage, oldPageInfo, newPageInfo, swapPageAddr, basePageSeq);
    }
}