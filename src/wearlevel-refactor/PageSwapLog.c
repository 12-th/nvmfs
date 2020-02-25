#include "Config.h"
#include "NVMOperations.h"
#include "PageSwapLog.h"

#define PAGE_SWAP_LOG_MAX_ENTRY_NUM (SIZE_2M / sizeof(struct PageSwapLogEntry))

void PageSwapLogFormat(struct PageSwapLog * log, nvm_addr_t metaDataAddr, nvm_addr_t addr)
{
    log->metaDataAddr = metaDataAddr;
    log->addr = addr;
    log->index = 0;
    NVMWrite(metaDataAddr, sizeof(addr), &addr);
}

void PageSwapLogUninit(struct PageSwapLog * log)
{
}

nvm_addr_t LogPageSwapBegin(struct PageSwapLog * log, physical_page_t pageA, physical_page_t pageB, nvm_addr_t tmp,
                            logical_page_t logicPageA, logical_page_t logicPageB, int type)
{
    nvm_addr_t addr;
    struct PageSwapLogEntry entry = {
        .type = type, .pageA = pageA, .pageB = pageB, .tmp = tmp, .logicPageA = logicPageA, .logicPageB = logicPageB};
    entry.step[0] = 1;
    addr = log->addr + sizeof(struct PageSwapLogEntry) * log->index;
    NVMWrite(addr, sizeof(entry), &entry);
    log->index++;
    if (log->index >= PAGE_SWAP_LOG_MAX_ENTRY_NUM)
        log->index = 0;
    return addr;
}

void LogPageSwapCommitStepx(nvm_addr_t addr, int step)
{
    char value = 1;
    nvm_addr_t stepAddr = addr + offsetof(struct PageSwapLogEntry, step) + step * sizeof(char);
    NVMWrite(stepAddr, sizeof(char), &value);
}

void LogPageSwapEnd(nvm_addr_t addr)
{
    int type = 0;

    NVMWrite(addr, sizeof(type), &type);
}

int PageSwapLogIsFull(struct PageSwapLog * log)
{
    return log->index == 0;
}

nvm_addr_t PageSwapLogGetAreaAddr(struct PageSwapLog * log)
{
    return log->addr;
}

void PageSwapLogSetAreaAddr(struct PageSwapLog * log, nvm_addr_t addr)
{
    NVMWrite(log->metaDataAddr, sizeof(nvm_addr_t), &addr);
    log->addr = addr;
}