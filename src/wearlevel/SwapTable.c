#include "NVMOperations.h"
#include "SwapTable.h"

void SwapTableInit(struct SwapTable * pTable, nvm_addr_t metaDataAddr)
{
    struct NVMSwapTable nvmData;

    NVMRead(metaDataAddr, sizeof(struct NVMSwapTable), &nvmData);

    pTable->nextBlock = nvmData.nextBlock;
    pTable->nextPage = nvmData.nextPage;
    pTable->swapBlocksNum = nvmData.swapBlocksNum;
    pTable->swapPagesNum = nvmData.swapPagesNum;
    pTable->swapBlocksAddr = nvmData.swapBlocksAddr;
    pTable->swapPagesAddr = nvmData.swapPagesAddr;
    pTable->metaDataAddr = metaDataAddr;
}

void SwapTableFormat(struct SwapTable * pTable, nvm_addr_t metaDataAddr, UINT32 swapBlocksNum, UINT32 swapPagesNum,
                     nvm_addr_t swapBlocksAddr, nvm_addr_t swapPagesAddr)
{
    struct NVMSwapTable nvmData = {.nextBlock = 0,
                                   .nextPage = 0,
                                   .swapBlocksNum = swapBlocksNum,
                                   .swapPagesNum = swapPagesNum,
                                   .swapBlocksAddr = swapBlocksAddr,
                                   .swapPagesAddr = swapPagesAddr};

    pTable->nextBlock = 0;
    pTable->nextPage = 0;
    pTable->swapBlocksNum = swapBlocksNum;
    pTable->swapPagesNum = swapPagesNum;
    pTable->swapBlocksAddr = swapBlocksAddr;
    pTable->swapPagesAddr = swapPagesAddr;
    pTable->metaDataAddr = metaDataAddr;
    NVMWrite(metaDataAddr, sizeof(struct NVMSwapTable), &nvmData);
}

nvm_addr_t GetSwapBlock(struct SwapTable * pTable)
{
    UINT32 index = pTable->nextBlock;
    pTable->nextBlock = (pTable->nextBlock + 1) % pTable->swapBlocksNum;
    return (index << BITS_2M) + pTable->swapBlocksAddr;
}

nvm_addr_t GetSwapPage(struct SwapTable * pTable)
{
    UINT32 index = pTable->nextPage;
    pTable->nextPage = (pTable->nextPage + 1) % pTable->swapPagesNum;
    return (index << BITS_4K) + pTable->swapPagesAddr;
}

void SwapTableUninit(struct SwapTable * pTable)
{
}
