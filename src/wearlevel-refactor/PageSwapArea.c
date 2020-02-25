#include "PageSwapArea.h"
#include "NVMOperations.h"

#define PAGE_SWAP_AREA_MAX_AREA_NUM 512

void PageSwapAreaFormat(struct PageSwapArea * pArea, nvm_addr_t addr, nvm_addr_t metaDataAddr)
{
    pArea->addr = addr;
    pArea->metaDataAddr = metaDataAddr;
    pArea->index = 0;
    NVMWrite(metaDataAddr, sizeof(addr), &addr);
}

void PageSwapAreaUninit(struct PageSwapArea * pArea)
{
}

nvm_addr_t PageSwapAreaGetTmpArea(struct PageSwapArea * pArea)
{
    nvm_addr_t addr;
    addr = pArea->addr + pArea->index * SIZE_4K;
    pArea->index++;
    if (pArea->index >= PAGE_SWAP_AREA_MAX_AREA_NUM)
        pArea->index = 0;
    return addr;
}

int PageSwapAreaIsFull(struct PageSwapArea * pArea)
{
    return pArea->index == 0;
}

nvm_addr_t PageSwapAreaGetAddr(struct PageSwapArea * pArea)
{
    return pArea->addr;
}

void PageSwapAreaSetAddr(struct PageSwapArea * pArea, nvm_addr_t addr)
{
    NVMWrite(pArea->metaDataAddr, sizeof(nvm_addr_t), &addr);
    pArea->addr = addr;
}