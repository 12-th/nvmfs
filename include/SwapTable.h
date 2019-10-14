#ifndef SWAP_TABLE_H
#define SWAP_TABLE_H

#include "Types.h"

struct NVMSwapTable
{
    UINT32 nextBlock;
    UINT32 swapBlockNum;
    nvm_addr_t swapTableAddr;
};

struct SwapTable
{
    UINT32 nextBlock;
    UINT32 swapBlockNum;
    nvm_addr_t metaDataAddr;
    nvm_addr_t swapTableAddr;
};

void SwapTableInit(struct SwapTable * pTable, nvm_addr_t metaDataAddr);
void SwapTableFormat(struct SwapTable * pTable, UINT32 swapBlockNum, UINT32 startBlock, nvm_addr_t metaDataAddr,
                     nvm_addr_t swapTableAddr);
void SwapTableUninit(struct SwapTable * pTable);
nvm_addr_t GetSwapBlock(struct SwapTable * pTable);

#endif