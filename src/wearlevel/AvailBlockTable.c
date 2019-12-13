#include "AvailBlockTable.h"
#include "BlockWearTable.h"
#include "NVMOperations.h"
#include <linux/mm.h>

static inline void AddBlockToHead(struct AvailBlockTable * pTable, int index, physical_block_t block)
{
    struct BlockList * cur;
    struct BlockList * next;
    struct BlockList * prev;

    cur = &pTable->blocks[block];
    next = pTable->heads[index].next;
    prev = &pTable->heads[index];
    cur->next = next;
    cur->prev = prev;
    prev->next = cur;
    next->prev = cur;
}

static inline void AddBlockToTail(struct AvailBlockTable * pTable, int index, physical_block_t block)
{
    struct BlockList * cur;
    struct BlockList * next;
    struct BlockList * prev;

    cur = &pTable->blocks[block];
    next = &pTable->heads[index];
    prev = next->prev;
    cur->next = next;
    cur->prev = prev;
    prev->next = cur;
    next->prev = cur;
}

static inline int IsBlockListEmpty(struct BlockList * list)
{
    return list->next == list;
}

static inline struct BlockList * RemoveOneFromBlockList(struct BlockList * head)
{
    struct BlockList *next, *cur;

    cur = head->next;
    next = cur->next;
    head->next = next;
    next->prev = head;
    return cur;
}

static inline void BlockListInit(struct BlockList * head)
{
    head->next = head->prev = head;
}

static inline void BlockSelfRemove(struct BlockList * block)
{
    struct BlockList *prev, *next;
    prev = block->prev;
    next = block->next;
    prev->next = next;
    next->prev = prev;
}

static inline void UpdateTableIndex(struct AvailBlockTable * pTable, int index)
{
    if (pTable->startIndex > index)
        pTable->startIndex = index;
    if (pTable->endIndex < index)
        pTable->endIndex = index;
}

static inline void AvailBlockTableInit(struct AvailBlockTable * pTable, UINT64 blockNum)
{
    int i;

    pTable->blockNum = blockNum;
    pTable->startIndex = 0;
    pTable->endIndex = 0;
    pTable->blocks = kvmalloc(sizeof(struct BlockList) * blockNum, GFP_KERNEL);
    pTable->heads = kvmalloc(sizeof(struct BlockList) * WEAR_STEP_NUMS, GFP_KERNEL);
    for (i = 0; i < WEAR_STEP_NUMS; ++i)
    {
        BlockListInit(&pTable->heads[i]);
    }
}

void AvailBlockTableRebuild(struct AvailBlockTable * pTable, struct BlockWearTable * pWearTable, UINT64 blockNum)
{
    int i;

    AvailBlockTableInit(pTable, blockNum);
    for (i = 0; i < blockNum; ++i)
    {
        UINT32 wearCount = BlockWearTableGet(pWearTable, i);
        UINT64 index = wearCount / STEP_WEAR_COUNT;

        AddBlockToTail(pTable, index, i);
        UpdateTableIndex(pTable, index);
    }
}

physical_block_t AvailBlockTableGetBlock(struct AvailBlockTable * pTable)
{
    int i;

    for (i = pTable->startIndex; i <= pTable->endIndex; ++i)
    {
        if (!IsBlockListEmpty(&pTable->heads[i]))
        {
            struct BlockList * ret = RemoveOneFromBlockList(&pTable->heads[i]);
            return ret - pTable->blocks;
        }
        pTable->startIndex = i;
    }
    return invalid_block;
}

void AvailBlockTablePutBlock(struct AvailBlockTable * pTable, physical_block_t block, UINT64 index)
{
    AddBlockToTail(pTable, index, block);
    UpdateTableIndex(pTable, index);
}

void AvailBlockTableUninit(struct AvailBlockTable * pTable)
{
    kvfree(pTable->blocks);
    kvfree(pTable->heads);
    pTable->blocks = NULL;
}

void AvailBlockTableSwapPrepare(struct AvailBlockTable * pTable, physical_block_t oldBlock, physical_block_t * newBlock)
{
    BlockSelfRemove(&pTable->blocks[oldBlock]);
    *newBlock = AvailBlockTableGetBlock(pTable);
}

void AvailBlockTableSwapEnd(struct AvailBlockTable * pTable, physical_block_t oldBlock, UINT64 oldBlockIndex,
                            physical_block_t newBlock, UINT64 newBlockIndex)
{
    AvailBlockTablePutBlock(pTable, oldBlock, oldBlockIndex);
    AvailBlockTablePutBlock(pTable, newBlock, newBlockIndex);
}