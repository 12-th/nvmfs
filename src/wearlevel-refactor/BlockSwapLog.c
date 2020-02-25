#include "BlockSwapLog.h"
#include "NVMOperations.h"

#define BLOCK_SWAP_LOG_MAX_ENTRY_NUM ((SIZE_2M) / sizeof(struct BlockSwapLogEntry))

static inline void FormatLogArea(nvm_addr_t addr)
{
    NVMemset(addr, 0, SIZE_2M);
}

void BlockSwapLogFormat(struct BlockSwapLog * log, nvm_addr_t metaDataAddr, nvm_addr_t addr)
{
    FormatLogArea(addr);
    NVMWrite(metaDataAddr, sizeof(addr), &addr);
    log->metaDataAddr = metaDataAddr;
    log->addr = addr;
    log->index = 0;
}

// void BlockSwapLogRebuild(struct BlockSwapLog * log, nvm_addr_t metaDataAddr)
// {
//     log->metaDataAddr = metaDataAddr;
//     NVMRead(metaDataAddr, sizeof(nvm_addr_t), &log->addr);
//     //遍历并重做没完成的那些交换操作
// }

void BlockSwapLogUninit(struct BlockSwapLog * log)
{
}

nvm_addr_t LogBlockSwapBegin(struct BlockSwapLog * log, physical_block_t blockA, physical_block_t blockB,
                             physical_block_t blockC, struct BlockInfo infoA, struct BlockInfo infoB,
                             struct BlockInfo infoC, int type)
{
    nvm_addr_t addr;
    struct BlockSwapLogEntry entry = {
        .blockA = blockA, .blockB = blockB, .blockC = blockC, .infoA = infoA, .infoB = infoB, .infoC = infoC};
    entry.step[0] = 1;
    addr = log->addr + log->index * sizeof(struct BlockSwapLogEntry);
    NVMWrite(addr, sizeof(entry), &entry);
    log->index++;
    if (log->index == BLOCK_SWAP_LOG_MAX_ENTRY_NUM)
        log->index = 0;
    entry.type = type;
    NVMWrite(addr, sizeof(entry.type), &entry.type);
    return addr;
}

void LogBlockSwapCommitStepx(nvm_addr_t addr, int step)
{
    char value = 1;
    nvm_addr_t stepAddr = addr + offsetof(struct BlockSwapLogEntry, step) + step * sizeof(char);
    NVMWrite(stepAddr, sizeof(char), &value);
}

void LogBlockSwapEnd(nvm_addr_t addr)
{
    int type = 0;

    NVMWrite(addr, sizeof(type), &type);
}

int BlockSwapLogIsFull(struct BlockSwapLog * log)
{
    return log->index == 0;
}

nvm_addr_t BlockSwapLogGetAreaAddr(struct BlockSwapLog * log)
{
    return log->addr;
}

void BlockSwapLogSetAreaAddr(struct BlockSwapLog * log, nvm_addr_t addr)
{
    NVMWrite(log->metaDataAddr, sizeof(nvm_addr_t), &addr);
    log->addr = addr;
    log->index = 0;
}