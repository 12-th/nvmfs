#ifndef BLOCK_SWAP_LOG_H
#define BLOCK_SWAP_LOG_H

#include "BlockUnmapTable.h"
#include "Types.h"

struct BlockSwapLogEntry
{
    int type;
    physical_block_t blockA;
    physical_block_t blockB;
    physical_block_t blockC;
    struct BlockInfo infoA;
    struct BlockInfo infoB;
    struct BlockInfo infoC;
    char step[4];
};

struct BlockSwapLog
{
    nvm_addr_t addr;
    nvm_addr_t metaDataAddr;
    int index;
};

void BlockSwapLogFormat(struct BlockSwapLog * log, nvm_addr_t metaDataAddr, nvm_addr_t addr);
void BlockSwapLogUninit(struct BlockSwapLog * log);
nvm_addr_t LogBlockSwapBegin(struct BlockSwapLog * log, physical_block_t blockA, physical_block_t blockB,
                             physical_block_t blockC, struct BlockInfo infoA, struct BlockInfo infoB,
                             struct BlockInfo infoC, int type);
void LogBlockSwapCommitStepx(nvm_addr_t addr, int step);
void LogBlockSwapEnd(nvm_addr_t addr);
int BlockSwapLogIsFull(struct BlockSwapLog * log);
nvm_addr_t BlockSwapLogGetAreaAddr(struct BlockSwapLog * log);
void BlockSwapLogSetAreaAddr(struct BlockSwapLog * log, nvm_addr_t addr);

#endif