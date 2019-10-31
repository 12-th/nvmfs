#ifndef SWAP_TABLE_H
#define SWAP_TABLE_H

#include "Types.h"

struct NVMSwapTable
{
    UINT32 nextBlock;
    UINT32 nextPage;
    UINT32 swapBlocksNum;
    UINT32 swapPagesNum;
    nvm_addr_t swapBlocksAddr;
    nvm_addr_t swapPagesAddr;
};

struct SwapTable
{
    UINT32 nextBlock;
    UINT32 nextPage;
    UINT32 swapBlocksNum;
    UINT32 swapPagesNum;
    nvm_addr_t swapBlocksAddr;
    nvm_addr_t swapPagesAddr;
    nvm_addr_t metaDataAddr;
};

void SwapTableInit(struct SwapTable * pTable, nvm_addr_t metaDataAddr);
void SwapTableFormat(struct SwapTable * pTable, nvm_addr_t metaDataAddr, UINT32 swapBlocksNum, UINT32 swapPagesNum,
                     nvm_addr_t swapBlocksAddr, nvm_addr_t swapPagesAddr);
void SwapTableUninit(struct SwapTable * pTable);
nvm_addr_t GetSwapBlock(struct SwapTable * pTable);
nvm_addr_t GetSwapPage(struct SwapTable * pTable);

#endif