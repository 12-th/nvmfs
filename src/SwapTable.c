#include "NVMOperations.h"
#include "SwapTable.h"


void SwapTableInit(struct SwapTable * pTable, nvm_addr_t metaDataAddr)
{
    struct NVMSwapTable nvmData;

    NVMRead(metaDataAddr, sizeof(struct NVMSwapTable), &nvmData);

    pTable->nextBlock = nvmData.nextBlock;
    pTable->swapBlockNum = nvmData.swapBlockNum;
    pTable->metaDataAddr = metaDataAddr;
    pTable->swapTableAddr = nvmData.swapTableAddr;
}

void SwapTableFormat(struct SwapTable * pTable, UINT32 swapBlockNum, UINT32 startBlock, nvm_addr_t metaDataAddr,
                     nvm_addr_t swapTableAddr)
{
    struct NVMSwapTable nvmData = {
        .nextBlock = startBlock, .swapBlockNum = swapBlockNum, .swapTableAddr = swapTableAddr};

    pTable->nextBlock = startBlock;
    pTable->swapBlockNum = swapBlockNum;
    pTable->metaDataAddr = metaDataAddr;
    pTable->swapTableAddr = swapTableAddr;
    NVMWrite(metaDataAddr, sizeof(struct NVMSwapTable), &nvmData);
}

nvm_addr_t GetSwapBlock(struct SwapTable * pTable)
{
    UINT32 ret = pTable->nextBlock;
    pTable->nextBlock = (pTable->nextBlock + 1) % pTable->swapBlockNum;
    return (ret << BITS_2M) + pTable->swapTableAddr;
}

void SwapTableUninit(struct SwapTable * pTable)
{
}
