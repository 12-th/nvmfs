#ifndef BLOCK_COLDNESS_MANAGER_H
#define BLOCK_COLDNESS_MANAGER_H

#include "Config.h"
#include "Types.h"
#include <linux/list.h>

struct BlockWearTable;

struct BlockColdnessManager
{
    struct list_head freeHeads[WEAR_STEP_NUMS];
    struct list_head busyHeads[WEAR_STEP_NUMS];
    struct list_head * nodes;
    int freeNodeNum;
    int busyNodeNum;
};

struct BlockColdnessManager * BlockColdnessManagerInit(UINT64 blockNum);
void BlockColdnessManagerUninit(struct BlockColdnessManager * manager);
struct BlockColdnessManager * BlockColdnessManagerFormat(UINT64 blockNum, int mustFreeBlockNum);
void BlockColdnessManagerRebuildNotifyBlockBusy(struct BlockColdnessManager * manager, physical_block_t block,
                                                UINT32 wearCount);
void BlockColdnessManagerRebuildEnd(struct BlockColdnessManager * manager, struct BlockWearTable * pTable,
                                    UINT32 blockNum);
void BlockColdnessManagerNotifyBlockBusy(struct BlockColdnessManager * manager, physical_block_t block, int index);
void BlockColdnessManagerNotifyBlockFree(struct BlockColdnessManager * manager, physical_block_t block, int index);
physical_block_t BlockColdnessManagerGetBusyBlock(struct BlockColdnessManager * manager, UINT32 index);
physical_block_t BlockColdnessManagerGetFreeBlock(struct BlockColdnessManager * manager, UINT32 index);
void BlockColdnessManagerRemove(struct BlockColdnessManager * manager, physical_block_t block);
void BlockColdnessManagerPut(struct BlockColdnessManager * manager, physical_block_t block, UINT32 index, int busy);

#endif