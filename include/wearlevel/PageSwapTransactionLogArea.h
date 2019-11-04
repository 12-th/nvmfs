#ifndef PAGE_SWAP_TRANSACTION_LOG_AREA_H
#define PAGE_SWAP_TRANSACTION_LOG_AREA_H

#include "PageUnmapTable.h"
#include "Types.h"

struct WearLeveler;

#define PAGE_SWAP_STEP1 1
#define PAGE_SWAP_STEP2 2
#define PAGE_SWAP_STEP3 3
#define PAGE_SWAP_STEP4 4

#define PAGE_SWAP_COMPLETE 0
#define PAGE_SWAP_QUICK 1

struct PageSwapTransactionInfoFlags
{
    UINT64 validFlag : 1;
    UINT64 swapType : 1;
    UINT64 step1 : 1;
    UINT64 step2 : 1;
    UINT64 step3 : 1;
    UINT64 step4 : 1;
};

struct PageSwapTransactionInfo
{
    physical_page_t oldPage;
    physical_page_t newPage;
    struct PageInfo oldPageInfo;
    struct PageInfo newPageInfo;
    nvm_addr_t swapPageAddr;
    struct PageSwapTransactionInfoFlags flags;
};

struct PageSwapTransactionLogArea
{
    nvm_addr_t addr;
    int index;
};

struct PageSwapTransaction
{
    nvm_addr_t addr;
    int index;
};

void PageSwapTransactionLogAreaFormat(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex);
void PageSwapTransactionLogAreaInit(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex);
void PageSwapTransactionLogAreaRecovery(struct PageSwapTransactionLogArea * pArea, nvm_addr_t addr);
void PageSwapTransactionLogAreaUninit(struct PageSwapTransactionLogArea * pArea);
void PageSwapTransactionInit(struct PageSwapTransaction * pTran, struct PageSwapTransactionLogArea * pArea);
void PageSwapTransactionUninit(struct PageSwapTransaction * pTran);
void DoPageQuickSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                                physical_page_t newPage, struct PageInfo * oldPageInfo, struct PageInfo * newPageInfo,
                                logical_page_t basePageSeq);
void DoPageCompleteSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                                   physical_page_t newPage, struct PageInfo * oldPageInfo,
                                   struct PageInfo * newPageInfo, nvm_addr_t swapPageAddr, logical_page_t basePageSeq);
void DoPageSwapTransaction(struct PageSwapTransaction * pTran, struct WearLeveler * wl, physical_page_t oldPage,
                           physical_page_t newPage, struct PageInfo * oldPageInfo, struct PageInfo * newPageInfo,
                           logical_page_t basePageSeq);

#endif