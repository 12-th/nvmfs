#ifndef PAGE_UNMAP_TABLE_H
#define PAGE_UNMAP_TABLE_H

#include "Types.h"

struct BlockUnmapTable;

struct NVMPageUnmapTableEntry
{
    UINT64 unmapPage0 : 9;
    UINT64 unmapPage1 : 9;
    UINT64 unmapPage2 : 9;
    UINT64 unmapPage3 : 9;
    UINT64 unmapPage4 : 9;
    UINT64 unmapPage5 : 9;
    UINT64 unmapPage0Busy : 1;
    UINT64 unmapPage1Busy : 1;
    UINT64 unmapPage2Busy : 1;
    UINT64 unmapPage3Busy : 1;
    UINT64 unmapPage4Busy : 1;
    UINT64 unmapPage5Busy : 1;
};

struct NVMPageUnmapTableEntryGroup
{
    struct NVMPageUnmapTableEntry entries[86];
};

struct NVMPageUnmapTable
{
    struct NVMPageUnmapTableEntryGroup groups[0];
};

struct PageUnmapTable
{
    nvm_addr_t addr;
    struct BlockUnmapTable * pBlockUnmapTable;
};

struct PageInfo
{
    logical_page_t unmapPage : 63;
    UINT64 busy : 1;
};

void PageUnmapTableFormat(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                          UINT64 pageNum, UINT64 tableSize);
void PageUnmapTableInit(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                        UINT64 pageNum);
void PageUnmapTableUninit(struct PageUnmapTable * pTable);
void PageUnmapTableGet(struct PageUnmapTable * pTable, physical_page_t pageSeq, struct PageInfo * info);
void PageUnmapTableSet(struct PageUnmapTable * pTable, physical_page_t pageSeq, struct PageInfo * info);
void PageUnmapTableBatchGet(struct PageUnmapTable * pTable, physical_block_t block, struct PageInfo * infos);
void PageUnmapTableBatchFormat(struct PageUnmapTable * pTable, physical_block_t block, UINT8 busy);

#endif