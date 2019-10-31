#ifndef BLOCK_SWAP_TRANSACTION_LOG_AREA_H
#define BLOCK_SWAP_TRANSACTION_LOG_AREA_H

#include "BlockUnmapTable.h"
#include "Config.h"
#include "Types.h"

struct WearLeveler;

#define BLOCK_SWAP_STEP1 1
#define BLOCK_SWAP_STEP2 2
#define BLOCK_SWAP_STEP3 3
#define BLOCK_SWAP_STEP4 4

#define BLOCK_SWAP_COMPLETE 0
#define BLOCK_SWAP_QUICK 1

struct BlockSwapTransactionInfoFlags
{
    UINT64 validFlag : 1;
    UINT64 swapType : 1;
    UINT64 step1 : 1;
    UINT64 step2 : 1;
    UINT64 step3 : 1;
    UINT64 step4 : 1;
};

struct BlockSwapTransactionInfo
{
    physical_block_t oldBlock;
    physical_block_t newBlock;
    nvm_addr_t swapBlockAddr;
    struct BlockInfo oldBlockInfo;
    struct BlockInfo newBlockInfo;
    union {
        struct BlockSwapTransactionInfoFlags flags;
        UINT64 data;
    };
} __attribute__((packed));

struct NVMBlockSwapTransactionLogArea
{
    struct BlockSwapTransactionInfo infos[BLOCK_SWAP_TRANSACTION_INFO_NUM_PER_AREA];
};

struct BlockSwapTransactionLogArea
{
    nvm_addr_t addr;
    int startIndex;
};

struct BlockSwapTransaction
{
    nvm_addr_t transactionInfoAddr;
    struct BlockSwapTransactionLogArea * pArea;
    int index;
};

void BlockSwapTransactionLogAreaFormat(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex);
void BlockSwapTransactionLogAreaInit(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr, int startIndex);
void BlockSwapTransactionLogAreaRecovery(struct BlockSwapTransactionLogArea * pArea, nvm_addr_t addr,
                                         struct WearLeveler * wl);
void BlockSwapTransactionLogAreaUninit(struct BlockSwapTransactionLogArea * pArea);
void BlockSwapTransactionInit(struct BlockSwapTransaction * pTran, struct BlockSwapTransactionLogArea * pArea);
void BlockSwapTransactionUninit(struct BlockSwapTransaction * pTran);
void DoBlockCompleteSwapTransaction(struct BlockSwapTransaction * pTran, struct WearLeveler * wl,
                                    physical_block_t oldBlock, physical_block_t newBlock, nvm_addr_t swapBlockAddr,
                                    struct BlockInfo * oldBlockInfo, struct BlockInfo * newBlockInfo);
void DoBlockQuickSwapTransaction(struct BlockSwapTransaction * pTran, struct WearLeveler * wl,
                                 physical_block_t oldBlock, physical_block_t newBlock, struct BlockInfo * oldBlockInfo,
                                 struct BlockInfo * newBlockInfo);
void DoBlockSwapTransaction(struct BlockSwapTransaction * pTran, struct WearLeveler * wl, physical_block_t oldBlock,
                            physical_block_t newBlock, struct BlockInfo * oldBlockInfo,
                            struct BlockInfo * newBlockInfo);

#endif