#include "Align.h"
#include "BlockUnmapTable.h"
#include "NVMOperations.h"
#include "PageUnmapTable.h"

static inline UINT64 GetUnmapPageRelativeIndexFromEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index)
{
    switch (index)
    {
    case 0:
        return entry->unmapPage0;
    case 1:
        return entry->unmapPage1;
    case 2:
        return entry->unmapPage2;
    case 3:
        return entry->unmapPage3;
    case 4:
        return entry->unmapPage4;
    case 5:
        return entry->unmapPage5;
    case 6:
        return entry->unmapPage6;
    }
    return 0;
}

static inline void SetUnmapPageRelativeIndexOfEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index, UINT64 value)
{
    switch (index)
    {
    case 0:
        entry->unmapPage0 = value;
    case 1:
        entry->unmapPage1 = value;
    case 2:
        entry->unmapPage2 = value;
    case 3:
        entry->unmapPage3 = value;
    case 4:
        entry->unmapPage4 = value;
    case 5:
        entry->unmapPage5 = value;
    case 6:
        entry->unmapPage6 = value;
    }
}

void PageUnmapTableFormat(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                          UINT64 pageNum, UINT64 tableSize)
{
    NVMemset(addr, 0, tableSize);
    pTable->addr = addr;
    pTable->pBlockUnmapTable = pBlockUnmapTable;
}

void PageUnmapTableInit(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                        UINT64 pageNum)
{
    pTable->addr = addr;
    pTable->pBlockUnmapTable = pBlockUnmapTable;
}

void PageUnmapTableUninit(struct PageUnmapTable * pTable)
{
}

logical_page_t PageUnmapTableGet(struct PageUnmapTable * pTable, physical_page_t pageSeq)
{
    logical_page_t logicPage;
    physical_block_t physBlock;
    logical_block_t logicBlock;
    struct BlockInfo info;
    nvm_addr_t addr;
    UINT64 offset;
    UINT64 offsetInsideEntry;
    UINT64 pageRelativeIndex;
    struct NVMPageUnmapTableEntry entry;

    pageRelativeIndex = pageSeq & ((1UL << (BITS_2M - BITS_4K)) - 1);
    physBlock = pageSeq >> (BITS_2M - BITS_4K);
    BlockUnmapTableGet(pTable->pBlockUnmapTable, physBlock, &info);
    logicBlock = info.unmapBlock;
    offset = logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup) +
             (pageRelativeIndex / 7) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 7;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
    logicPage = (logicBlock << (BITS_2M - BITS_4K)) + GetUnmapPageRelativeIndexFromEntry(&entry, offsetInsideEntry);

    return logicPage;
}

void PageUnmapTableSet(struct PageUnmapTable * pTable, physical_page_t pageSeq, UINT64 relativeIndex)
{
    physical_block_t physBlock;
    logical_block_t logicBlock;
    struct BlockInfo info;
    nvm_addr_t addr;
    UINT64 offset;
    UINT64 offsetInsideEntry;
    UINT64 pageRelativeIndex;
    struct NVMPageUnmapTableEntry entry;

    pageRelativeIndex = pageSeq & ((1UL << (BITS_2M - BITS_4K)) - 1);
    physBlock = pageSeq >> (BITS_2M - BITS_4K);
    BlockUnmapTableGet(pTable->pBlockUnmapTable, physBlock, &info);
    logicBlock = info.unmapBlock;
    offset = logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup) +
             (pageRelativeIndex / 7) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 7;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
    SetUnmapPageRelativeIndexOfEntry(&entry, offsetInsideEntry, relativeIndex);
    NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
}

void PageUnmapTableBatchGet(struct PageUnmapTable * pTable, physical_block_t block, UINT64 * relativeIndice)
{
    logical_block_t logicBlock;
    struct BlockInfo info;
    nvm_addr_t addr;
    struct NVMPageUnmapTableEntry entry;
    int i, j;
    UINT64 relativeIndex;

    BlockUnmapTableGet(pTable->pBlockUnmapTable, block, &info);
    logicBlock = info.unmapBlock;
    addr = pTable->addr + logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup);
    for (i = 0, j = 0; i < 512; ++i)
    {
        if (j == 0)
            NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
        relativeIndex = GetUnmapPageRelativeIndexFromEntry(&entry, j);
        relativeIndice[i] = relativeIndex;
        addr += sizeof(struct NVMPageUnmapTableEntry);
        ++j;
        if (j >= 7)
            j = 0;
    }
}