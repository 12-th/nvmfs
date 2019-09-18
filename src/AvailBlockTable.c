#include "AvailBlockTable.h"
#include "BlockWearTable.h"
#include "NVMOperations.h"
#include "SuperBlock.h"
#include <linux/mm.h>

static void BlockBufferInit(struct BlockBuffer * pBuffer)
{
    pBuffer->size = 0;
    pBuffer->capacity = 0;
    pBuffer->blocks = NULL;
}
static void AvailBlockTableInit(struct AvailBlockTable * pTable)
{
    pTable->buffers = NULL;
}

static void AssureBufferHasEnoughCapacity(struct AvailBlockTable * pTable, UINT64 index)
{
    struct BlockBuffer * pBuffer = &pTable->buffers[index - pTable->startIndex];
    if (pBuffer->blocks == NULL)
    {
        pBuffer->blocks = kvmalloc(sizeof(block_t) * 1024, GFP_KERNEL);
        pBuffer->size = 0;
        pBuffer->capacity = 1024;
    }
    else if (pBuffer->size == pBuffer->capacity)
    {
        block_t * newBlocks = kvmalloc(sizeof(block_t) * 2 * pBuffer->capacity, GFP_KERNEL);
        int i;
        for (i = 0; i < pBuffer->size; ++i)
        {
            newBlocks[i] = pBuffer->blocks[i];
        }
        kvfree(pBuffer->blocks);
        pBuffer->blocks = newBlocks;
        pBuffer->capacity = pBuffer->capacity * 2;
    }
}

static void AssureTableHasEnoughCapacity(struct AvailBlockTable * pTable, UINT64 index)
{
    int i;
    if (pTable->buffers == NULL)
    {
        pTable->startIndex = pTable->endIndex = index;
        pTable->buffers = kvmalloc(sizeof(struct BlockBuffer), GFP_KERNEL);
        BlockBufferInit(&pTable->buffers[0]);
    }
    else if (index < pTable->startIndex)
    {
        struct BlockBuffer * newBuffers =
            kvmalloc((pTable->endIndex - index + 1) * sizeof(struct BlockBuffer), GFP_KERNEL);
        int base = pTable->startIndex - index;
        for (i = 0; i < base; ++i)
        {
            BlockBufferInit(&newBuffers[i]);
        }
        for (i = base; i < pTable->endIndex - index + 1; ++i)
        {
            newBuffers[i] = pTable->buffers[i - base];
        }
        kvfree(pTable->buffers);
        pTable->buffers = newBuffers;
        pTable->startIndex = index;
    }
    else if (index > pTable->endIndex)
    {
        struct BlockBuffer * newBuffers =
            kvmalloc((index - pTable->startIndex + 1) * sizeof(struct BlockBuffer), GFP_KERNEL);
        int base = pTable->endIndex - pTable->startIndex + 1;
        for (i = 0; i < base; ++i)
        {
            newBuffers[i] = pTable->buffers[i];
        }
        for (i = base; i < index - pTable->startIndex + 1; ++i)
        {
            BlockBufferInit(&newBuffers[i]);
        }
        kvfree(pTable->buffers);
        pTable->buffers = newBuffers;
        pTable->endIndex = index;
    }
}

static void InitAllSpinlocks(struct AvailBlockTable * pTable)
{
    int i;
    for (i = pTable->startIndex; i <= pTable->endIndex; ++i)
    {
        spin_lock_init(pTable->buffers[i - pTable->startIndex].lock);
    }
}

static void AddBlock(struct AvailBlockTable * pTable, block_t block, UINT64 index)
{
    struct BlockBuffer * pBuffer;
    AssureTableHasEnoughCapacity(pTable, index);
    AssureBufferHasEnoughCapacity(pTable, index);
    pBuffer = &pTable->buffers[index - pTable->startIndex];
    pBuffer->blocks[pBuffer->size] = block;
    pBuffer->size++;
}

static block_t RemoveBlock(struct BlockBuffer * pBuffer)
{
    block_t block = pBuffer->blocks[pBuffer->size - 1];
    pBuffer->size--;
    return block;
}

static UINT64 RemoveBlocks(struct AvailBlockTable * pTable, UINT64 index, block_t * blocks, UINT64 size)
{
    struct BlockBuffer * pBuffer;
    int num = 0;

    pBuffer = &pTable->buffers[index - pTable->startIndex];
    while (size && pBuffer->size)
    {
        *blocks = RemoveBlock(pBuffer);
        blocks++;
        num++;
    }
    return num;
}

static nvm_addr_t SerializeBlockBuffer(struct BlockBuffer * pBuffer, nvm_addr_t addr, UINT64 index)
{
    NVMWrite(addr, sizeof(index), &index);
    addr += sizeof(index);
    NVMWrite(addr, sizeof(pBuffer->size), &pBuffer->size);
    addr += sizeof(pBuffer->size);
    for (int i = 0; i < pBuffer->size; ++i)
    {
        NVMWrite(addr, sizeof(block_t), &pBuffer->blocks[i]);
        addr += sizeof(block_t);
    }

    return addr;
}

void AvailBlockTableSerialize(struct AvailBlockTable * pTable, nvm_addr_t pAvailBlockTable)
{
    int i, limit;
    nvm_addr_t addr = pAvailBlockTable;
    UINT64 endFlag = -1;

    limit = pTable->endIndex - pTable->startIndex + 1;
    for (i = 0; i < limit; ++i)
    {
        struct BlockBuffer * pBuffer = &pTable->buffers[i];
        if (!pBuffer->blocks || !pBuffer->size)
            continue;
        addr = SerializeBlockBuffer(pBuffer, addr, i + pTable->startIndex);
    }

    NVMWrite(addr, sizeof(endFlag), &endFlag);
}
void AvailBlockTableDeserialize(struct AvailBlockTable * pTable, nvm_addr_t pAvailBlockTable)
{
    UINT64 index;
    nvm_addr_t addr = pAvailBlockTable;

    AvailBlockTableInit(pTable);
    NVMRead(addr, sizeof(index), &index);
    addr += sizeof(index);
    while (index != -1)
    {
        UINT32 size;
        int i;
        NVMRead(addr, sizeof(size), &size);
        addr += sizeof(size);

        for (i = 0; i < size; ++i)
        {
            block_t block;
            NVMRead(addr, sizeof(block_t), &block);
            AddBlock(pTable, block, index);
            addr += sizeof(block_t);
        }

        NVMRead(addr, sizeof(index), &index);
        addr += sizeof(index);
    }
    InitAllSpinlocks(pTable);
}

void AvailBlockTableRebuild(struct AvailBlockTable * pTable, struct BlockWearTable * pWearTable, UINT64 blockNum)
{
    UINT64 i;

    AvailBlockTableInit(pTable);

    for (i = 0; i < blockNum; ++i)
    {
        UINT32 wearCount = GetBlockWearCount(pWearTable, i);
        AddBlock(pTable, i, wearCount / STEP_WEAR_COUNT);
    }
    InitAllSpinlocks(pTable);
}

UINT64 AvailBlockTableGetBlocks(struct AvailBlockTable * pTable, block_t * buffer, int blockNum)
{
    int i, limit;
    UINT64 gotBlocksNum = 0;

    limit = pTable->endIndex - pTable->startIndex + 1;
    for (i = 0; i < limit; ++i)
    {
        UINT64 n = RemoveBlocks(pTable, i + pTable->startIndex, buffer, blockNum);
        blockNum -= n;
        gotBlocksNum += n;
        buffer += n;
        if (!blockNum)
            return gotBlocksNum;
    }
    return gotBlocksNum;
}

void AvailBlockTablePutBlocks(struct AvailBlockTable * pTable, block_t block, UINT64 index)
{
    AddBlock(pTable, block, index);
}

void AvailBlockTableUninit(struct AvailBlockTable * pTable)
{
    struct BlockBuffer * pBuffer;
    if (pTable->buffers)
    {
        int i;
        int limit;

        limit = pTable->endIndex - pTable->startIndex + 1;
        for (i = 0; i < limit; ++i)
        {
            pBuffer = &pTable->buffers[i];
            if (pBuffer->blocks)
            {
                kvfree(pBuffer->blocks);
            }
        }
        kvfree(pTable->buffers);
    }
}