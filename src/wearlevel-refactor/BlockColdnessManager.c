#include "BlockColdnessManager.h"
#include "BlockWearTable.h"
#include <linux/mm.h>

struct BlockColdnessManager * BlockColdnessManagerInit(UINT64 blockNum)
{
    struct BlockColdnessManager * manager;
    int i;

    manager = kvmalloc(sizeof(struct BlockColdnessManager), GFP_KERNEL);
    manager->nodes = kvmalloc(sizeof(struct list_head) * blockNum, GFP_KERNEL);

    for (i = 0; i < WEAR_STEP_NUMS; ++i)
    {
        INIT_LIST_HEAD(&manager->freeHeads[i]);
    }
    for (i = 0; i < WEAR_STEP_NUMS; ++i)
    {
        INIT_LIST_HEAD(&manager->busyHeads[i]);
    }
    for (i = 0; i < blockNum; ++i)
    {
        INIT_LIST_HEAD(&manager->nodes[i]);
    }

    manager->freeNodeNum = 0;
    manager->busyNodeNum = 0;

    return manager;
}

struct BlockColdnessManager * BlockColdnessManagerFormat(UINT64 blockNum, int mustFreeBlockNum)
{
    struct BlockColdnessManager * manager;
    int i;

    manager = BlockColdnessManagerInit(blockNum);
    for (i = 0; i < blockNum - mustFreeBlockNum; ++i)
    {
        list_add_tail(&manager->nodes[i], &manager->freeHeads[0]);
    }
    return manager;
}

void BlockColdnessManagerUninit(struct BlockColdnessManager * manager)
{
    kvfree(manager->nodes);
    kvfree(manager);
}

void BlockColdnessManagerRebuildNotifyBlockBusy(struct BlockColdnessManager * manager, physical_block_t block,
                                                UINT32 wearCount)
{
    UINT32 index = wearCount / STEP_WEAR_COUNT;
    list_add_tail(&manager->nodes[block], &manager->busyHeads[index]);
}

void BlockColdnessManagerRebuildEnd(struct BlockColdnessManager * manager, struct BlockWearTable * pTable,
                                    UINT32 blockNum)
{
    int i;
    UINT32 wearCount;

    for (i = 0; i < blockNum; ++i)
    {
        if (list_empty(&manager->nodes[i]))
        {
            wearCount = BlockWearTableGet(pTable, i);
            list_add_tail(&manager->nodes[i], &manager->freeHeads[wearCount / STEP_WEAR_COUNT]);
        }
    }
}

void BlockColdnessManagerNotifyBlockBusy(struct BlockColdnessManager * manager, physical_block_t block, int index)
{
    list_del(&manager->nodes[block]);
    list_add_tail(&manager->nodes[block], &manager->busyHeads[index]);
}

void BlockColdnessManagerNotifyBlockFree(struct BlockColdnessManager * manager, physical_block_t block, int index)
{
    list_del(&manager->nodes[block]);
    list_add_tail(&manager->nodes[block], &manager->freeHeads[index]);
}

physical_block_t BlockColdnessManagerGetBusyBlock(struct BlockColdnessManager * manager, UINT32 index)
{
    struct list_head * node;
    if (list_empty(&manager->busyHeads[index]))
        return invalid_block;
    node = manager->busyHeads[index].next;
    list_del(node);
    return node - manager->nodes;
}

physical_block_t BlockColdnessManagerGetFreeBlock(struct BlockColdnessManager * manager, UINT32 index)
{
    struct list_head * node;
    if (list_empty(&manager->freeHeads[index]))
        return invalid_block;
    node = manager->freeHeads[index].next;
    list_del(node);
    return node - manager->nodes;
}

void BlockColdnessManagerRemove(struct BlockColdnessManager * manager, physical_block_t block)
{
    list_del(&manager->nodes[block]);
}

void BlockColdnessManagerPut(struct BlockColdnessManager * manager, physical_block_t block, UINT32 index, int busy)
{
    if (busy)
    {
        list_add_tail(&manager->nodes[block], &manager->busyHeads[index]);
    }
    else
    {
        list_add_tail(&manager->nodes[block], &manager->freeHeads[index]);
    }
}