#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include "MapTable.h"
#include "Types.h"

struct BlockColdnessManager;
struct BlockUnmapTable;
struct PageUnmapTable;
struct BlockWearTable;
struct BlockSwapLog;
struct MapTable;
struct WearLeveler;

struct BlockManager
{
    UINT64 blockNum;
    UINT64 dataStartOffset;
    struct BlockColdnessManager * coldness;
    struct BlockUnmapTable * unmapTable;
    struct PageUnmapTable * pageUnmapTable;
    struct BlockWearTable * wearTable;
    struct PageWearTable * pageWearTable;
    struct BlockSwapLog * log;
    struct MapTable * mapTable;
    UINT64 threshold;
    nvm_addr_t thresholdAddr;
};

void BlockManagerInit(struct BlockManager * manager, struct WearLeveler * wl);
void BlockManagerRead(struct BlockManager * manager, logic_addr_t addr, UINT64 size, void * buffer);
void BlockManagerWrite(struct BlockManager * manager, logic_addr_t addr, UINT64 size, void * buffer,
                       int increaseWearCount);
void BlockManagerMemset(struct BlockManager * manager, logic_addr_t addr, UINT64 size, int value,
                        int increaseWearCount);
void BlockManagerTrim(struct BlockManager * manager, logic_addr_t addr);
void BlockManagerSplit(struct BlockManager * manager, logic_addr_t addr);
void BlockManagerMerge(struct BlockManager * manager, logic_addr_t addr);

void BlockManagerIncreaseWearCount(struct BlockManager * manager, nvm_addr_t physAddr, UINT32 increaseWearCount);
void BlockManagerIncreaseSpecialBlockWearCount(struct BlockManager * manager, UINT32 threshold, nvm_addr_t physAddr,
                                               nvm_addr_t * newPhysAddr);

#endif