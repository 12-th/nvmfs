#ifndef PAGE_SWAP_LOG_H
#define PAGE_SWAP_LOG_H

#include "Types.h"

struct PageSwapLogEntry
{
    int type;
    physical_page_t pageA;
    physical_page_t pageB;
    nvm_addr_t tmp;
    logical_page_t logicPageA;
    logical_page_t logicPageB;
    char step[4];
};

struct PageSwapLog
{
    nvm_addr_t addr;
    nvm_addr_t metaDataAddr;
    int index;
};

void PageSwapLogFormat(struct PageSwapLog * log, nvm_addr_t metaDataAddr, nvm_addr_t addr);
void PageSwapLogUninit(struct PageSwapLog * log);
nvm_addr_t LogPageSwapBegin(struct PageSwapLog * log, physical_page_t pageA, physical_page_t pageB, nvm_addr_t tmp,
                            logical_page_t logicPageA, logical_page_t logicPageB, int type);
void LogPageSwapCommitStepx(nvm_addr_t addr, int step);
void LogPageSwapEnd(nvm_addr_t addr);
int PageSwapLogIsFull(struct PageSwapLog * log);
nvm_addr_t PageSwapLogGetAreaAddr(struct PageSwapLog * log);
void PageSwapLogSetAreaAddr(struct PageSwapLog * log, nvm_addr_t addr);

#endif