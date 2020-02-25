#ifndef PAGE_SWAP_AREA_H
#define PAGE_SWAP_AREA_H

#include "Types.h"

struct PageSwapArea
{
    nvm_addr_t addr;
    nvm_addr_t metaDataAddr;
    int index;
};

void PageSwapAreaFormat(struct PageSwapArea * pArea, nvm_addr_t addr, nvm_addr_t metaDataAddr);
void PageSwapAreaUninit(struct PageSwapArea * pArea);
nvm_addr_t PageSwapAreaGetTmpArea(struct PageSwapArea * pArea);
int PageSwapAreaIsFull(struct PageSwapArea * pArea);
nvm_addr_t PageSwapAreaGetAddr(struct PageSwapArea * pArea);
void PageSwapAreaSetAddr(struct PageSwapArea * pArea, nvm_addr_t addr);

#endif